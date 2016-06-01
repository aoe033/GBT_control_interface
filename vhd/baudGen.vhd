-----------------------------------------------------------------------------------------
-- baud rate generator for uart 
--
-- this module has been changed to receive the baud rate dividing counter from registers.
-- the two registers should be calculated as follows:
-- first register:
--              baud_freq = 16*baud_rate / gcd(global_clock_freq, 16*baud_rate)
-- second register:
--              baud_limit = (global_clock_freq / gcd(global_clock_freq, 16*baud_rate)) - baud_freq 
--
-- Original by: Arild Velure
-- Modified by: Anders Østevik
-----------------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.uart_gbt_pkg.all;

entity baudGen is
  port ( 
  clk:      in  std_logic;              -- global clock input
  reset_n:  in  std_logic;              -- global reset input
  ce16:     out std_logic               -- baud rate multiplyed by 16
  );                    
end baudGen;

architecture Behavioral of baudGen is

  signal counter : unsigned(15 downto 0);

  begin
    -- baud divider counter
    -- clock divider output
    process (reset_n, clk)
    begin
      if (reset_n = '0') then
        counter <= (others => '0');
        ce16 <= '0';
      elsif (rising_edge(clk)) then
        if (counter >= C_BAUDLIMIT) then
          counter <= counter - C_BAUDLIMIT; 
			 -- Supposed to pulse a '1' every 27 clock cycles: 
			 -- #1 When counter = 26.13 * 576 ~ 27 * 576 = 15552 -> counter = 503
			 -- #2 When counter = (26 * 576) + 503 = 15479 -> counter = 430
			 -- #3 When counter = (26 * 576) + 430 = 15406 -> counter = 357

          ce16 <= '1';
        else
          counter <= counter + C_BAUDFREQ; 
          ce16 <= '0';
        end if;
      end if;
    end process;
  end Behavioral;
