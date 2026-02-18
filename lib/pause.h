#include <iostream>

#ifdef _WIN32
    #include <conio.h>   // for _getch()
#else
    #include <termios.h>
    #include <unistd.h>
#endif

// Cross-platform getch()
char getch() {
#ifdef _WIN32
    return _getch();
#else
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(STDIN_FILENO, &old) < 0)
        perror("tcgetattr");
    old.c_lflag &= ~ICANON;  // disable buffered I/O
    old.c_lflag &= ~ECHO;    // disable echo
    if (tcsetattr(STDIN_FILENO, TCSANOW, &old) < 0)
        perror("tcsetattr");
    if (read(STDIN_FILENO, &buf, 1) < 0)
        perror("read");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old) < 0)
        perror("tcsetattr");
    return buf;
#endif
}

void pressAnyKeyToContinue() {
    std::cout << "Press any key to continue..." << std::flush;
    getch();  // waits for a single key press
}

int main() {
    std::cout << "Hello, world!\n";
    pressAnyKeyToContinue();
    std::cout << "\nContinuing program...\n";
    return 0;
}

