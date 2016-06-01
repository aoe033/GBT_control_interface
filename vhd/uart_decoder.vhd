library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

use work.uart_gbt_pkg.all;

entity uart_decoder is

port(
  clk:       in std_logic;  -- clock
  reset_n:   in std_logic;  -- asynchronous reset
  
  s_tick:    in std_logic;
  
  u2t:       in t_u2t;   -- Signals from uart in to top
  t2u:       out t_t2u;  -- Signals from top out to uart
  
  led : out std_logic_vector(7 downto 0) -- LEDS for debugging
);
end uart_decoder;

architecture arch of uart_decoder is

signal gbt_sw_reg, gbt_sw_next : std_logic_vector(35 downto 0) := (others=>'0');
signal gbt_probes_reg : std_logic_vector(29 downto 0) := (others=>'0');
signal gbt_swpr_reg : std_logic_vector(65 downto 0) := (others=>'0');

type state_type is (idle, read1, wait1, read2); -- State machine
signal state_reg, state_next : state_type;

type store_type is array (1 downto 0) of std_logic_vector(C_DBIT-1 downto 0);
signal b_reg, b_next : store_type; -- Registers to store the two incoming bytes.

signal t_reg, t_next : unsigned(7 downto 0); -- Timeout register for counting in wait state. Counts up to (16 * 10 bits) + 1 = time it takes to receive second address byte

signal arst_reg, arst_next : unsigned(11 downto 0); -- Timeout-reset register. If there is no respons from the uart in given amount of time, reset the uart. Counts up to (16 * 100 bits) = 10 bytes.
signal uart_arst : std_logic := '1';

--SIGNALS USED TO WRITE TO THE GBT SIGNAL VECTORS
--signal wr_en : std_logic;   -- Write-enable registers (See state machine)
--signal wr_data_reg, wr_data_next, wr_value : std_logic;   -- 
--signal wr_value : std_logic;   -- 
--signal wr_adr : std_logic_vector(7 downto 0) := (others=>'0');

signal t2u_i : t_t2u; -- Internal version of output
----------------------------------------------------------------------------
-- t_t2u:
    -- FIFO Read in data:        
    --    wo_rd_u               : std_logic;     
    -- FIFO Read out data:       
    --    wo_wr_u               : std_logic;   
    --    wo_wdata              : std_logic_vector(C_DBIT-1 downto 0);
----------------------------------------------------------------------------   

begin

  -- Assigning internally used signals to outputs
  t2u <= t2u_i;
  
  --Testverdier
  gbt_probes_reg <= "100000010000000000000101000001";
  --gbt_switches   <= "111111111111111111111111111111111111";

  gbt_swpr_reg(35 downto 0) <= gbt_sw_reg;
  gbt_swpr_reg(65 downto 36) <= gbt_probes_reg;

      ----------------------------------------------Process for writing to Switch-vector--------------------------------------------      
------------------------------------------------------------------------------------------------------------------------------------------
----  This process writes to gbt_switches when the uart-receiver has received a write-byte followed by an address-byte.
----  The following signal requirements needs to be met, and will be set in the state machine: 
----  wr_en = '1', wr_data = '1', wr_adr = r_data, 
----  wr_value = '0' or '1' when write-byte is C_REQ_WRITE_0 or C_REQ_WRITE_1 respectively.
------------------------------------------------------------------------------------------------------------------------------------------
      ------------------------------------------------------------------------------------------------------------------------------      
      
  --writeSwitches:process(clk, reset_n)
  --begin
  --  if(reset_n = '0')then
  --     gbt_switches <= (others=>'0');
  --     ram.rdadr <= (others=>'0');
  --     ram_adrinc <= (others=>'0');
  --  elsif rising_edge(clk)then
  --    if(ram_adrinc < x"23") then
  --      ram_adrinc <= ram_adrinc + 1;
  --    else
  --      ram_adrinc <= (others=>'0');
  --    end if;
  --    ram.rdadr <= std_logic_vector(ram_adrinc);
  --    gbt_switches(to_integer(unsigned(ram.rdadr))) <= ram.rdata(0);
  --  end if;
  --end process;

  --  writeSwitches:process(clk, reset_n)
  --begin
  --  if(reset_n = '0')then
  --     gbt_switches <= (others=>'0');
  --  elsif rising_edge(clk)then
  --    if (wr_en = '1') then
  --      if (wr_adr < x"24") then
  --        gbt_switches(to_integer(unsigned(wr_adr))) <= wr_value;
  --      end if;
  --    end if;
  --  end if;
  --end process;
  
      ----------------------------------------------Process for determining the states--------------------------------------------      
----------------------------------------------------------------------------------------------------------------------------------------
----  idle -> Set all signals to default. If any byte is received and stored in to the rx-fifo, read it out from u2t.ro_rdata and go to read1 state.
----  read1 -> Check the first received byte. u2t.ro_rdata must be equal to a read- or write-request. 
----    - If true, go to wait-state. Else, go to idle state.
----    - If (byte = C_REQ_WRITE_0 or C_REQ_WRITE_1), wr_en = '1' and wr_value = '0' or '1'.
----  wait1 -> Wait for next byte. Time period must be equal to that time it takes to receive a byte, i.e (16 * 27 * 115200) + 1 clk cycle for a baud rate of 115200.
----    If no byte has been received (rx_empty = 1), go to idle. If a byte has been received, go to read2 state.
----  read2 -> Check the second received byte. u2t.ro_rdata must be equal to a legal address (x00 -> xC1). 
----    - If true, return to wait state.
----      - If wr_en = '1', Write to gbt_switches with given address from u2t.ro_rdata. (See process for writing to gbt_switches)
----      - If wr_en = '0', Send data with given address from u2t.ro_rdata to tx-fifo. 
----------------------------------------------------------------------------------------------------------------------------------------
      ----------------------------------------------------------------------------------------------------------------------------
  
  -- FSMD state & data registers
  fsmd_registers: process(clk, reset_n)
  begin
    if (reset_n = '0') then
      state_reg <= idle;            -- Start state
      t_reg <= (others => '0');     -- Timing register
      b_reg(0) <= (others => '0');  -- Received byte1 register
      b_reg(1) <= (others => '0');  -- Received byte2 register
      arst_reg <= (others => '0');  -- Timing register
      t2u_i.wo_arst_n <= '0';       -- Reset Uart
      
      gbt_sw_reg <= (others => '0'); -- Reset Switch-register
      
    elsif (rising_edge(clk)) then
      state_reg <= state_next;
      t_reg <= t_next;
      --b_reg(0) <= b_next(0);
      --b_reg(1) <= b_next(1);      
      b_reg <= b_next; 
      arst_reg <= arst_next;
      t2u_i.wo_arst_n <= uart_arst; -- Uart reset_n held high
      gbt_sw_reg <= gbt_sw_next; -- Register for gbt-switches/sources
    end if;
  end process;

  -- next-state logic and data path functional units/routing
  state_machine: process(state_reg, t_reg, b_reg, s_tick, u2t, gbt_swpr_reg, arst_reg, gbt_sw_reg)
  begin
    -- Assign default values to signals and states
    state_next <= state_reg;
    t_next <= t_reg;
    b_next <= b_reg;
    arst_next <= arst_reg;
    uart_arst <= '1'; -- Uart areset_n held high
    gbt_sw_next <= gbt_sw_reg;  -- Register for gbt-switches/sources
    
    t2u_i.wo_rd_u <= '0';
    t2u_i.wo_wr_u <= '0';
    t2u_i.wo_wdata <= (others => '0');

    --wr_en <= '0';
    wr_adr <= (others => '0');
    wr_value <= '0';
  
    
    led <= (others => '1');
    
    case state_reg is
    -- IDLE STATE
      when idle =>
        if (u2t.ro_rxempty = '0') then-- Rx-fifo has received one byte, and is no longer empty 
          state_next <= read1;
          t2u_i.wo_rd_u <= '1';
          b_next(0) <= u2t.ro_rdata;-- Set b_next register to input signal
          arst_next <= (others => '0'); -- Timing register
        else
          if (s_tick = '1') then -- wait for Baud generator tick
            if (arst_reg = x"fff") then
              uart_arst <= '0'; -- Reset Uart for one clock cycle
              led(7) <= '0';
              arst_next <= (others => '0'); -- Timing register
            else
              arst_next <= arst_reg + 1;
            end if;
          end if;
          b_next <= (others => (others => '0')); 
        end if;
        led(6 downto 0) <= "1111110";
        
      -- READ1 STATE
      when read1 =>
        state_next <= wait1;
        t_next <= (others => '0');
        case b_reg(0) is -- Analyse received byte. Must contain a request (read, write)
          when C_REQ_READ_0 => -- no op, requirements have been met
         -- when C_REQ_READ_1 => -- no op, requirements have been met
          when C_REQ_WRITE_0 => -- no op, requirements have been met
          when C_REQ_WRITE_1 => -- no op, requirements have been met
          when others =>
            state_next <= idle;
        end case;
       led(6 downto 0) <= "1111110";
        
      when wait1 =>
        if (s_tick = '1') then -- wait for Baud generator tick
          if (t_reg = C_TIMEOUT) then -- TIMEOUT! 160 ticks is the amount of time (baud gen ticks, 16 ticks * 10 bits) it takes before the following address byte arrives
              t_next <= (others => '0');
              state_next <= idle;
          end if;
          if (u2t.ro_rxempty = '0') then -- Rx-fifo has received one byte, and it is no longer empty 
            state_next <= read2;
            t2u_i.wo_rd_u <= '1';
            b_next(1) <= u2t.ro_rdata; -- Set b_next register to ro_rdata
          end if;
          t_next <= t_reg + 1; -- keep counting until t_reg = 255
        end if;
        led(6 downto 0) <= "1111110";
        
       -- READ2 STATE
      when read2 =>
        led(6 downto 0) <= "1111110";
        state_next <= idle;
        case b_reg(1) is -- Analyse received byte. Must contain an address
          when C_REQ_READ_0 => -- no op: byte is not an address. Return to idle.
          --when C_REQ_READ_1 => -- no op: byte is not an address. Return to idle.
          when C_REQ_WRITE_0 => -- no op: byte is not an address. Return to idle.
          when C_REQ_WRITE_1 => -- no op: byte is not an address. Return to idle.
          when others =>
            t2u_i.wo_wdata(7) <= gbt_swpr_reg(to_integer(unsigned(b_reg(1))));
            t2u_i.wo_wdata(6 downto 0) <= b_reg(1)(6 downto 0);
            wr_adr <= b_reg(1);
            --ram.wradr <= b_reg(1)(5 downto 0);
            case b_reg(0) is
              when C_REQ_READ_0 =>
                t2u_i.wo_wr_u <= '1';
              --when C_REQ_READ_1 =>
                --t2u_i.wo_wr_u <= '1';
              when C_REQ_WRITE_0 =>
                --wr_en <= '1';
                wr_value <= '0';
                if (wr_adr < x"24") then
                  if (gbt_sw_reg(to_integer(unsigned(wr_adr))) /= wr_value) then
                    gbt_sw_next(to_integer(unsigned(wr_adr))) <= wr_value;
                  end if;
                end if;
              when C_REQ_WRITE_1 =>
               -- wr_en <= '1';
                wr_value <= '1';           
                if (wr_adr < x"24") then
                  if (gbt_sw_reg(to_integer(unsigned(wr_adr))) /= wr_value) then
                    gbt_sw_next(to_integer(unsigned(wr_adr))) <= wr_value;
                  end if;
                end if;
              when others => -- no op
            end case;
        end case;
    end case;
end process;


end arch;