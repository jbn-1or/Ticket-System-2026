#include "Command.hpp"
#include "System.hpp"
#include <iostream>
#include <string>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    ticket::CommandParser parser;
    ticket::TicketSystem system;
    system.initialize("data");

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.size() == 0)
            continue;
        ticket::Command cmd = parser.parse(line);
        std::string out = system.execute(cmd);
        std::cout << "[" << cmd.timestamp << "] " << out << '\n';
        if (cmd.type == ticket::CommandType::Exit)
            break;
    }

    return 0;
}