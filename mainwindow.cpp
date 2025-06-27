#include "mainwindow.h"
#include <QVBoxLayout>
#include <QLabel>
#include <sstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("C++ Code Optimizer");
    resize(1000, 800);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QLabel *inputLabel = new QLabel("Input C++ Code:", this);
    inputTextEdit = new QTextEdit(this);
    inputTextEdit->setAcceptDrops(false);
    inputTextEdit->setPlaceholderText("Drag and drop a .cpp file here or type your code directly...");

    QLabel *outputLabel = new QLabel("Optimized Output:", this);
    outputTextEdit = new QTextEdit(this);
    outputTextEdit->setReadOnly(true);
    outputTextEdit->setPlaceholderText("Optimized code will appear here...");

    runButton = new QPushButton("Optimize Code", this);
    runButton->setStyleSheet("QPushButton { padding: 8px; font-weight: bold; }");

    mainLayout->addWidget(inputLabel);
    mainLayout->addWidget(inputTextEdit);
    mainLayout->addWidget(outputLabel);
    mainLayout->addWidget(outputTextEdit);
    mainLayout->addWidget(runButton, 0, Qt::AlignRight);

    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    setAcceptDrops(true);

    connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunButtonClicked);
}

MainWindow::~MainWindow()
{
}

std::string MainWindow::qtStringToStd(const QString &qtStr) {
    return qtStr.toStdString();
}

QString MainWindow::stdStringToQt(const std::string &stdStr) {
    return QString::fromStdString(stdStr);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        foreach (const QUrl &url, event->mimeData()->urls()) {
            QString fileName = url.toLocalFile();
            if (fileName.endsWith(".cpp", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    QString filePath = urls.first().toLocalFile();
    loadFile(filePath);
    event->acceptProposedAction();
}

void MainWindow::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open the file");
        return;
    }

    QTextStream in(&file);
    inputTextEdit->setPlainText(in.readAll());
    setWindowTitle(QString("C++ Code Optimizer - %1").arg(filePath));
    file.close();
}

QString MainWindow::optimizeCode(const QString &code)
{
    optimizer.loadCode(qtStringToStd(code));
    optimizer.analyze();
    optimizer.optimize();
    return stdStringToQt(optimizer.getOptimizedCode());
}

void MainWindow::onRunButtonClicked()
{
    QString inputCode = inputTextEdit->toPlainText();
    if (inputCode.isEmpty()) {
        QMessageBox::warning(this, "Error", "Input code is empty!");
        return;
    }

    outputTextEdit->setPlainText(optimizeCode(inputCode));
}