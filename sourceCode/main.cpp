#include "mainwindow.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <QApplication>

using namespace std;
int main(int argc, char *argv[])
{
    string password;

    cout << "Введите ваш пароль для sudo: ";
    getline(cin, password);

    // Формируем команду для выполнения с sudo
    string command = "echo '" + password + "' | sudo -S whoami 2>/dev/null";

    // Выполняем команду и проверяем результат
    int result = system(command.c_str());

    if (result == 0) {
        cout << "Успешный вход в sudo!" << endl;
        // Здесь можно выполнять команды, требующие sudo
    } else {
        cout << "Неверный пароль или ошибка sudo!" << endl;
        return 1;
    }
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
