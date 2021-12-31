idf_component_register(SRCS "quell.c" "protocol.c" "FIFO.c" "main.c" "ProtocolTask/protocolTask.c" "TerminalTask/terminalTask.c" "crc.c"
                    INCLUDE_DIRS "." "ProtocolTask" "TerminalTask")
