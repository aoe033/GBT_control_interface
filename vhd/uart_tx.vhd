----------------------------------------------------------------------------------------
-- uart transmitter 
-- Based on the design by Pong P. Chu's design
-----------------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.uart_gbt_pkg.all;

entity uart_tx is
	port(
      clk:            in  std_logic;    -- global clock input
      reset_n:        in  std_logic;	-- global reset_n input
      tx_start:       in std_logic;
      tx:             out  std_logic;   -- serial data output
      s_tick:         in std_logic;     -- enable tick from baud rate generator
      tx_done_tick:   out std_logic;    -- signaling when transmission is done                    
      din:            in std_logic_vector (C_DBIT-1 downto 0)	-- data input
	);                         
end uart_tx;

architecture arch of uart_tx is
	
  type state_type is (idle, start, data, stop);
  signal state_reg, state_next 	: state_type;
	
  signal s_reg, s_next            : unsigned(3 downto 0);	-- registers to keep track of the number of sampling ticks
  signal n_reg, n_next            : unsigned(2 downto 0);	-- registers to keep track of data bits transmitted in the data state
  signal b_reg, b_next            : std_logic_vector(C_DBIT-1 downto 0); -- registers to reassemble the transmitted bits
  signal tx_reg, tx_next          : std_logic;
	
begin

-- FSMD state & data registers
  fsmd_register: process(clk, reset_n)
  begin
    if (reset_n = '0') then
      state_reg <= idle;
      s_reg <= (others => '0');
      n_reg <= (others => '0');
      b_reg <= (others => '0');
      tx_reg <= '1';	-- # set transmission line to idle
    elsif (rising_edge(clk)) then
      state_reg <= state_next;
      s_reg <= s_next;
      n_reg <= n_next;
      b_reg <= b_next;
      tx_reg <= tx_next;	-- # Listen to tx_next
    end if;
  end process;

-- next-state logic and data path functional units/routing
  state_machine: process(state_reg, s_reg, n_reg, b_reg, s_tick, tx_reg, tx_start, din)
  begin
    state_next <= state_reg;
    s_next <= s_reg;
    n_next <= n_reg;
    b_next <= b_reg;
    tx_next <= tx_reg;
    tx_done_tick <= '0';
		
    case state_reg is
    -- IDLE STATE
      when idle =>
        tx_next <= '1';	-- # set tx to idle
        if (tx_start = '1') then	-- # Receive start signal, go to start-state 
          state_next <= start;
          s_next <= (others => '0');	-- # prepare tick-counter
          b_next <= din;	-- # Set b_next register to input signal
        end if;
    -- START STATE
      when start =>
       tx_next <= '0';	-- # set start bit
       if (s_tick = '1') then	-- # wait for Baud generator tick
         if (s_reg = 15) then	-- # 
           state_next <= data;
           s_next <= (others => '0');	-- # reset_n tick-counter
           n_next <= (others => '0');	-- # prepare bit-counter
         else
           s_next <= s_reg + 1;		-- # keep counting up to 7 (reach middle of start bit)
         end if;
       end if;
-- DATA STATE
      when data =>
        tx_next <= b_reg(0);	-- # capture the data bit (from LSB to MSB)
        if (s_tick = '1') then -- # wait for Baud generator tick
          if (s_reg = 15) then -- # Wait for signal to reach middle of data bit, then transmit
            s_next <= (others => '0');	-- # reset_n tick-counter
            b_next <= '0' & b_reg(7 downto 1);	-- # Shift the data from MSB to LSB
            if (n_reg = (C_DBIT-1)) then	-- # Proceed to stop-state when all C_DBIT-7 are transmitted
              state_next <= stop;
            else
              n_next <= n_reg + 1;	-- # keep counting up to C_DBIT-1
            end if;
          else
            s_next <= s_reg + 1;	-- # keep counting up to 15
          end if;
        end if;
-- STOP STATE
      when stop =>
        tx_next <= '1';	-- # apply stop bit
        if (s_tick = '1') then
          if (s_reg = (C_SB_TICK-1)) then -- # Proceed to idle-state when all stopbits are transmitted
            state_next <= idle;
            tx_done_tick <= '1';	-- # Signal that the signal has been succesfully transmitted
          else
            s_next <= s_reg + 1;	-- # Keep counting untill C_SB_TICK-1
          end if;
        end if;
    end case;
end process;
	
tx <= tx_reg;	-- # apply value to tx-transmission signal

end arch;
