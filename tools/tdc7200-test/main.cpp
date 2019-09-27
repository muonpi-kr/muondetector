#include <QCoreApplication>
#include <pigpiodhandler.h>
#include <tdc7200.h>
#include <iostream>
#include <QVector>
#include <QDebug>
#include <termios.h>
#include <unistd.h>

const unsigned int intb_pin = 8;

int getch() {
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qRegisterMetaType<std::string>("std::string");
    PigpiodHandler pighandler(QVector<unsigned int>{intb_pin});
    TDC7200 tdc(8);
    pighandler.connect(&pighandler, &PigpiodHandler::spiData, &tdc, &TDC7200::onDataReceived);
    pighandler.connect(&tdc, &TDC7200::readData, &pighandler, &PigpiodHandler::readSpi);
    pighandler.connect(&tdc, &TDC7200::writeData, &pighandler, &PigpiodHandler::writeSpi);
    pighandler.connect(&pighandler, &PigpiodHandler::signal, &tdc, &TDC7200::onDataAvailable);

    std::cout << "press a key to start tdc measurement" << std::endl;
    while(true){
        getch(); // wait for keypress
        // here send start measurement
        tdc.startMeas();
    }
    return a.exec();
}
