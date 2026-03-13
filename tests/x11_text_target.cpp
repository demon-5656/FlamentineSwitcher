#include <cstdio>

#include <QApplication>
#include <QFile>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QString outputPathFromArgs(const QStringList& arguments) {
    if (arguments.size() >= 2 && !arguments.at(1).trimmed().isEmpty()) {
        return arguments.at(1).trimmed();
    }

    return QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/flamentine-x11-target.out");
}

bool writeResult(const QString& outputPath, const QString& text) {
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    outputFile.write(text.toUtf8());
    outputFile.flush();
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("x11_text_target"));
    QApplication::setApplicationDisplayName(QStringLiteral("Flamentine X11 E2E Target"));

    const QString outputPath = outputPathFromArgs(app.arguments());

    QWidget window;
    window.setWindowTitle(QStringLiteral("Flamentine X11 E2E Target"));
    window.resize(520, 120);

    auto* lineEdit = new QLineEdit(&window);
    lineEdit->setObjectName(QStringLiteral("targetLineEdit"));
    lineEdit->setPlaceholderText(QStringLiteral("Type text here"));
    lineEdit->setText(QStringLiteral("seed"));
    lineEdit->selectAll();

    auto* layout = new QVBoxLayout(&window);
    layout->addWidget(lineEdit);

    const auto finish = [&app, &window, lineEdit, outputPath]() {
        writeResult(outputPath, lineEdit->text());
        window.hide();
        app.quit();
    };

    QObject::connect(lineEdit, &QLineEdit::returnPressed, &window, finish);
    QTimer::singleShot(15000, &window, finish);

    window.show();
    window.raise();
    window.activateWindow();

    QTimer::singleShot(250, &window, [&window, lineEdit]() {
        lineEdit->setFocus(Qt::ActiveWindowFocusReason);
        lineEdit->selectAll();
        std::printf("%llu\n", static_cast<unsigned long long>(window.winId()));
        std::fflush(stdout);
    });

    return app.exec();
}
