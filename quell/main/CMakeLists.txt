idf_component_register(SRCS "main.c" "FIFO.c" "FIFOUart.c"  "ProtocolTask/protocolTask.c" "ProtocolTask/protocol.c" "TerminalTask/terminalTask.c" "TerminalTask/terminal.c" "crc.c" "quell.c"
                        INCLUDE_DIRS "." "ProtocolTask" "TerminalTask")
