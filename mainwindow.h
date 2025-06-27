#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QFile>
#include <QUrl>
#include <QMimeData>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QRegularExpression>
#include <QStringList>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <regex>

class CppOptimizer {
private:
    std::set<std::string> usedFunctions;
    std::set<std::string> definedFunctions;
    std::map<std::string, std::vector<std::string>> functionCalls;
    std::set<std::string> usedVariables;
    std::set<std::string> definedVariables;
    std::vector<std::string> lines;
    std::set<std::string> keepFunctions = {"main"};
    std::regex commentRegex = std::regex(R"(\/\*.*?\*\/|\/\/[^\n]*)");
    bool inGlobalScope = true;
    int currentBraceLevel = 0;

public:
    void skipFunction(size_t& i) {
        int braceCount = 1;
        i++;
        
        while (i < lines.size() && braceCount > 0) {
            std::string line = lines[i];
            braceCount += std::count(line.begin(), line.end(), '{');
            braceCount -= std::count(line.begin(), line.end(), '}');
            i++;
        }
        
        if (i < lines.size()) i--;
    }

    bool isWhitespaceOrComment(const std::string& line) {
        std::string clean = std::regex_replace(line, commentRegex, "");
        return std::all_of(clean.begin(), clean.end(), [](char c) { 
            return isspace(c) || c == '\0'; 
        });
    }

    void loadCode(const std::string& code) {
        std::istringstream iss(code);
        std::string line;
        while (std::getline(iss, line)) {
            if (!isWhitespaceOrComment(line)) {
                lines.push_back(line);
            }
        }
    }

    std::string getOptimizedCode() {
        std::ostringstream oss;
        for (const auto& line : lines) {
            oss << line << "\n";
        }
        return oss.str();
    }

    void analyze() {
        std::string currentFunction;

        std::regex funcDefRegex(R"((?:\w+(?:\s*::\s*\w+)*\s+)+(\w+)\s*\([^)]*\)\s*(?:const)?\s*\{?)");
        std::regex funcCallRegex(R"(\b(\w+)\s*\([^)]*\))");
        std::regex varDefRegex(R"((\w+(?:\s*::\s*\w+)*)\s+(\w+)\s*(?:=\s*[^;]+)?\s*;)");
        std::regex varUseRegex(R"(\b([a-zA-Z_]\w*)\b(?=\s*\())");

        for (size_t i = 0; i < lines.size(); i++) {
            std::string line = lines[i];
            std::string cleanLine = std::regex_replace(line, commentRegex, "");

            size_t openBraces = std::count(cleanLine.begin(), cleanLine.end(), '{');
            size_t closeBraces = std::count(cleanLine.begin(), cleanLine.end(), '}');
            currentBraceLevel += openBraces - closeBraces;
            inGlobalScope = (currentBraceLevel == 0);

            std::smatch funcMatch;
            if (std::regex_search(cleanLine, funcMatch, funcDefRegex)) {
                std::string funcName = funcMatch[1].str();
                definedFunctions.insert(funcName);
                currentFunction = funcName;
                functionCalls[funcName] = std::vector<std::string>();
            }

            std::sregex_iterator funcIt(cleanLine.begin(), cleanLine.end(), funcCallRegex);
            std::sregex_iterator funcEnd;
            for (; funcIt != funcEnd; ++funcIt) {
                std::string calledFunc = (*funcIt)[1].str();
                if (!currentFunction.empty() && definedFunctions.count(calledFunc)) {
                    functionCalls[currentFunction].push_back(calledFunc);
                }
            }

            std::smatch varMatch;
            if (std::regex_search(cleanLine, varMatch, varDefRegex)) {
                std::string varName = varMatch[2].str();
                definedVariables.insert(varName);
                if (!inGlobalScope) {
                    usedVariables.insert(varName);
                }
            }

            if (!inGlobalScope && !currentFunction.empty()) {
                std::sregex_iterator varIt(cleanLine.begin(), cleanLine.end(), varUseRegex);
                for (; varIt != std::sregex_iterator(); ++varIt) {
                    std::string varName = (*varIt)[1].str();
                    if (definedVariables.count(varName)) {
                        usedVariables.insert(varName);
                    }
                }
            }
        }

        findUsedFunctions("main");
    }

    void findUsedFunctions(const std::string& funcName) {
        if (usedFunctions.count(funcName) || !definedFunctions.count(funcName)) return;
        usedFunctions.insert(funcName);
        for (const auto& calledFunc : functionCalls[funcName]) {
            if (calledFunc != funcName) {
                findUsedFunctions(calledFunc);
            }
        }
    }

    void optimize() {
        std::vector<std::string> optimizedLines;
        std::string currentFunction;
        inGlobalScope = true;
        currentBraceLevel = 0;
        
        std::regex funcDefRegex(R"((?:\w+(?:\s*::\s*\w+)*\s+)+(\w+)\s*\([^)]*\)\s*(?:const)?\s*\{?)");
        std::regex varDefRegex(R"((\w+(?:\s*::\s*\w+)*)\s+(\w+)\s*(?:=\s*[^;]+)?\s*;)");

        for (size_t i = 0; i < lines.size(); i++) {
            std::string line = lines[i];
            std::string cleanLine = std::regex_replace(line, commentRegex, "");
            
            size_t openBraces = std::count(cleanLine.begin(), cleanLine.end(), '{');
            size_t closeBraces = std::count(cleanLine.begin(), cleanLine.end(), '}');
            currentBraceLevel += openBraces - closeBraces;
            inGlobalScope = (currentBraceLevel == 0);

            std::smatch funcMatch;
            if (std::regex_search(cleanLine, funcMatch, funcDefRegex)) {
                std::string funcName = funcMatch[1].str();
                if (usedFunctions.count(funcName) || keepFunctions.count(funcName)) {
                    optimizedLines.push_back(line);
                    currentFunction = funcName;
                } else {
                    skipFunction(i);
                }
                continue;
            }

            std::smatch varMatch;
            if (std::regex_search(cleanLine, varMatch, varDefRegex)) {
                std::string varName = varMatch[2].str();
                if (!usedVariables.count(varName)) {
                    continue;
                }
            }

            optimizedLines.push_back(line);
        }

        lines = optimizedLines;
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onRunButtonClicked();

private:
    QTextEdit *inputTextEdit;
    QTextEdit *outputTextEdit;
    QPushButton *runButton;
    CppOptimizer optimizer;
    
    void loadFile(const QString &filePath);
    QString optimizeCode(const QString &code);
    std::string qtStringToStd(const QString &qtStr);
    QString stdStringToQt(const std::string &stdStr);
};

#endif