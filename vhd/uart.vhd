library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.uart_gbt_pkg.all;

entity uart is
    port(
        clk:                    in std_logic;
        --reset_n:                in std_logic;
    -- UART interface
        t2u:                    in t_t2u;
        u2t:                    out t_u2t;
    -- UART Receiver/Transmitter lines
        rx:                     in std_logic;
        tx:                     out std_logic;      
    -- Baud Generator tick signals      
        s_tick:                 in std_logic
    );
end uart;

architecture str_arch of uart is
    --signal tick:                  std_logic;
    signal rx_done_tick:            std_logic;
    signal tx_fifo_out:             std_logic_vector(C_DBIT-1 downto 0);
    signal rx_data_out:             std_logic_vector(C_DBIT-1 downto 0);
    signal tx_empty:                std_logic; 
    signal tx_fifo_not_empty:       std_logic;
    signal tx_done_tick:            std_logic;

    signal u2t_i: t_u2t; -- Internal version of output

begin

  -- Assigning internally used signals to outputs
  u2t <= u2t_i;
                        
  uart_rx_unit : entity work.uart_rx(arch)
      port map(
          clk            => clk, 
          reset_n        => t2u.wo_arst_n, 
          rx             => rx,          
          s_tick         => s_tick, 
          rx_done_tick   => rx_done_tick,
          dout           => rx_data_out
      );
    
  fifo_rx_unit: entity work.fifo_buffer(arch)
      port map(
          clk            => clk, 
          reset_n        => t2u.wo_arst_n, 
          wr             => rx_done_tick, 
          w_data         => rx_data_out, 
          rd             => t2u.wo_rd_u, 
          r_data         => u2t_i.ro_rdata, 
          Empty          => u2t_i.ro_rxempty, 
          Full           => open
      );   

  fifo_tx_unit: entity work.fifo_buffer(arch)
      port map(
          clk            => clk, 
          reset_n        => t2u.wo_arst_n, 
          wr             => t2u.wo_wr_u, 
          w_data         => t2u.wo_wdata, 
          rd             => tx_done_tick, 
          r_data         => tx_fifo_out, 
          Empty          => tx_empty, 
          Full           => u2t_i.ro_txfull
      );   
    
                        
  uart_tx_unit: entity work.uart_tx(arch)
      port map(
          clk            => clk, 
          reset_n        => t2u.wo_arst_n, 
          tx_start       => tx_fifo_not_empty,
          tx             => tx, 
          s_tick         => s_tick, 
          tx_done_tick   => tx_done_tick, 
          din            => tx_fifo_out
          );
                        
  tx_fifo_not_empty <= not tx_empty;
  
end str_arch;