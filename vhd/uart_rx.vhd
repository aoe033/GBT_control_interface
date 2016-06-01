----------------------------------------------------------------------------------------
-- uart receiver 
-- Based on the design by Pong P. Chu's design
-----------------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.uart_gbt_pkg.all;

entity uart_rx is
    port(
      clk:              in  std_logic;  -- global clock input
      reset_n:          in  std_logic;  -- global reset_n input
      rx:               in std_logic;  -- serial data input
      s_tick:           in std_logic;   -- enable tick from baud rate generator
      rx_done_tick:     out std_logic;  -- Flags when transmission is done                    
      dout:             out std_logic_vector (C_DBIT-1 downto 0)  -- data output
    );                         
end uart_rx;

architecture arch of uart_rx is
    
    type state_type is (idle, start, data, stop);
    signal state_reg, state_next    : state_type;   
    signal s_reg, s_next            : unsigned(3 downto 0); -- registers to keep track of the number of sampling ticks
    signal n_reg, n_next            : unsigned(2 downto 0); -- registers to keep track of data bits received in the data state
    signal b_reg, b_next            : std_logic_vector(C_DBIT-1 downto 0); -- registers to reassemble the received bits

begin

-- FSMD state & data registers
  fsmd_register: process(clk, reset_n)
  begin
    if (reset_n = '0') then
      state_reg <= idle;
      s_reg <= (others => '0');
      n_reg <= (others => '0');
      b_reg <= (others => '0');
    elsif (rising_edge(clk)) then
      state_reg <= state_next;
      s_reg <= s_next;
      n_reg <= n_next;
      b_reg <= b_next;
    end if;
  end process;

-- next-state logic and data path functional units/routing
  state_machine: process(state_reg, s_reg, n_reg, b_reg, s_tick, rx)
  begin
    state_next <= state_reg;
    s_next <= s_reg;
    n_next <= n_reg;
    b_next <= b_reg;
    rx_done_tick <= '0';
    
    case state_reg is
    -- IDLE STATE
      when idle =>
        if (rx = '0') then  -- Receives a start bit, go to start-state 
          state_next <= start;
          s_next <= (others => '0');    -- prepare tick-counter
        end if;
    -- START STATE
      when start =>
        if (s_tick = '1') then  -- wait for Baud generator tick
          if (s_reg = 7) then   -- Wait for signal to reach middle of start bit, then start the data-state
            state_next <= data;
            s_next <= (others => '0');  -- reset_n tick-counter
            n_next <= (others => '0');  -- prepare bit-counter
          else
            s_next <= s_reg + 1;        -- keep counting up to 7 (reach middle of start bit)
          end if;
        end if;
    -- DATA STATE
      when data =>
        if (s_tick = '1') then -- wait for Baud generator tick
          if (s_reg = 15) then -- Wait for signal to reach middle of data bit, then sample
            s_next <= (others => '0');  -- reset_n tick-counter
            b_next <= rx & b_reg(7 downto 1);   -- Shift in the received serial data
            if (n_reg = (C_DBIT-1)) then  -- Proceed to stop-state when all C_DBIT-7 are sampled
              state_next <= stop;
            else
              n_next <= n_reg + 1;  -- keep counting up to C_DBIT-1
            end if;
          else
             s_next <= s_reg + 1;   -- keep counting up to 15
          end if;
        end if;
    -- STOP STATE
      when stop =>
        if (s_tick = '1') then
          if (s_reg = (C_SB_TICK-1)) then -- Proceed to idle-state when all stopbits are sampled
            state_next <= idle;
            rx_done_tick <= '1';    -- Signal that the signal has been succesfully received
          else
            s_next <= s_reg + 1;    -- Keep counting untill C_SB_TICK-1
          end if;
        end if;
    end case;
  end process;
    
    dout <= b_reg;  -- Finally assert word in dout-register

end arch;
