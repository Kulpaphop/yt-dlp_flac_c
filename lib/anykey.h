#ifndef ANYKEY_HPP
#define ANYKEY_HPP

#include <iostream>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
    #include <conio.h>
#else
    #include <termios.h>
    #include <unistd.h>
#endif

/// Waits for a key press with a custom message.
/// Usage: anykey("Press any key to continue...");
inline void anykey(const std::string& message = "Press any key to continue...") {
    std::cout << message;
    std::cout.flush();

#if defined(_WIN32) || defined(_WIN64)
    _getch(); // Windows: waits for any key
#else
    // POSIX: disable canonical mode and echo
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    getchar(); // wait for one character

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // restore settings
#endif
    std::cout << "\n";
}

#endif // ANYKEY_HPP
