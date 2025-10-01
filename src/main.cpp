#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QLabel label("Hello, World!");
    label.resize(200, 100);              // Set window size
    label.setAlignment(Qt::AlignCenter); // Center text
    label.show();

    return app.exec();
}
