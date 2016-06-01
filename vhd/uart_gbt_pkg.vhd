
------------------------------------------------------------------------------------------
-- VHDL unit     : UART Library : uart_gbt_pkg
--
-- Description   : Contains constants and records linked with uart and gbt signals
------------------------------------------------------------------------------------------

Library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


package uart_gbt_pkg is

  -- Notation for regs: (Included in constant name as info to SW)
  -- - RW: Readable and writable reg.
  -- - RO: Read only reg. (output from IP)
  -- - WO: Write only reg. (typically single cycle strobe to IP)

  -- Notation for signals (or fields in record) going between PIF and core:
  -- Same notations as for register-constants above, but
  -- a preceeding 'a' (e.g. awo) means the register is auxiliary to the PIF.
  -- This means no flop in the PIF, but in the core. (Or just a dummy-register with no flop)
  
  
------------------------------------------------------------------------------------------
-- Description   : Contains constants and records linked with uart and gbt signals
------------------------------------------------------------------------------------------

  -- Uart settings:
  -- Standard settings: 115200 baudrate, 8 databits, 1 stopbit, 2^6 FIFO width
  constant C_DBIT:                   integer := 8; -- Databits contained in transmission (8 is standard)
  constant C_SB_TICK:                integer := 16; -- Number of baudgen ticks for stop bits (16 is standard). 16/24/32 for 1/1.5/2 stop bits
  constant C_FIFO_W:                 integer := 8; -- Width of uart FIFOs (rx and tx). Can hold 2^(FIFO_W-1) words in FIFO at any time. 

------------------------------------------------------------------------------------------
-- Baud Rate Constants Table for 50Mhz global clock:      
--                                     BAUDFREQ        BAUDLIMIT
-- Baud Rate:     
--                 9600                 x"030"           x"3cd9"  
--                19200                 x"060"           x"3ccd"  
--                57600                 x"120"           x"3be9"
--               115200                 x"240"           x"3ac9"  
--               230400                 x"480"           x"3889"
--               500000                 x"004"           x"3d05"
--              1152000                 x"480"           x"3889"
------------------------------------------------------------------------------------------
  constant C_BAUDFREQ:               unsigned(11 downto 0) := x"060"; 
  constant C_BAUDLIMIT:              unsigned(15 downto 0) := x"3ccd";
  
------------------------------------------------------------------------------------------
-- Description   : REQ-constants on fpga side must be equal to REQ-constants on the PC side
------------------------------------------------------------------------------------------
  constant C_REQ_READ_0:             std_logic_vector(C_DBIT-1 downto 0) := x"DD";
  constant C_REQ_READ_1:             std_logic_vector(C_DBIT-1 downto 0) := x"DD";

  constant C_REQ_WRITE_0:            std_logic_vector(C_DBIT-1 downto 0) := x"EE";
  constant C_REQ_WRITE_1:            std_logic_vector(C_DBIT-1 downto 0) := x"FF";

------------------------------------------------------------------------------------------
-- Description   : REQ-constants on fpga side must be equal to REQ-constants on the PC side
------------------------------------------------------------------------------------------
  constant C_TIMEOUT:            unsigned(7 downto 0) := x"FF"; --x"FF";

  -- Signals from uart to top
   type t_u2t is record
    -- FIFO Read in data        
        ro_rdata              : std_logic_vector(C_DBIT-1 downto 0);      
    -- FIFO empty/full signals
        ro_txfull             : std_logic;
        ro_rxempty            : std_logic;
   end record t_u2t;
   
  -- Signals from top to uart
   type t_t2u is record
   -- Asynchronous reset_n signal
        wo_arst_n               : std_logic;
    -- FIFO Read in data        
        wo_rd_u               : std_logic;     
    -- FIFO Read out data       
        wo_wr_u               : std_logic;   
        wo_wdata              : std_logic_vector(C_DBIT-1 downto 0);
   end record t_t2u;   
   
   
  -- Ram signals (Used internally in uart_decoder to store switch data)
   type t_ram is record
    -- Ram Write data
        wradr              : std_logic_vector(5 downto 0);
        wdata              : std_logic_vector(0 downto 0);
        wren               : std_logic;
    -- Ram Read data        
        rdadr              : std_logic_vector(5 downto 0);
        rdata              : std_logic_vector(0 downto 0);
   end record t_ram;

end package uart_gbt_pkg;

