library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

use work.uart_gbt_pkg.all;

entity uart_top is
port(
  clk:       in std_logic;  -- clock
  reset_n:   in std_logic;  -- asynchronous reset
  rx:        in std_logic;  -- Receiver line
  tx:        out std_logic; -- Transmitter line
  
  led : out std_logic_vector(7 downto 0) -- LEDS for debugging
);
end uart_top;

architecture rtl of uart_top is

-- Baud Generator tick
signal tick : std_logic;

-- Uart interface
signal t2u: t_t2u;
signal u2t: t_u2t;

begin

uart_decoder : entity work.uart_decoder(arch)
    port map(
        clk            => clk, 
        reset_n        => reset_n, 
    -- Uart interface
        t2u            => t2u,
        u2t            => u2t,
    -- Baud Generator tick signals
        s_tick         => tick,
        
        led            => led
    );

uart_unit : entity work.uart(str_arch)
    port map(
        clk            => clk, 
       -- reset_n        => reset_n, 
    -- Uart interface
        t2u            => t2u,
        u2t            => u2t,
    -- UART Receiver/Transmitter lines
        rx             => rx, 
        tx             => tx, 
    -- Baud Generator tick signals
        s_tick         => tick
    );
  
baud_gen_unit : entity work.baudGen(Behavioral)
    port map(
        clk            => clk, 
        reset_n        => reset_n,
        ce16           => tick
    );


end rtl;