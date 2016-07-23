/*
 * Copyright 2010 and beyond, Juan Luis Paz
 *
 * This file is part of Script Report.
 *
 * Script Report is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Script Report is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Script Report.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "shell.h"

#include <QScriptValueIterator>

#include "scriptable/scriptableshellengine.h"

class ShellPrivate {
public:
    ShellPrivate() :
        lineNumber(1),
        isInitialized(false),
        useGlobalEngine(true),
        exit(false),
        exitCode(0),
        fileName(QString()),
        scriptableEngine(0)
    {}

    int lineNumber;
    QScriptEngine *engine;
    bool isInitialized;
    bool useGlobalEngine;
    bool exit;
    int exitCode;
    QString fileName;
    ScriptableShellEngine *scriptableEngine;
};

/*
 * Static
 */

static QScriptValue print(QScriptContext *context, QScriptEngine *engine) {
    QScriptValue calleeData = context->callee().data();
    Shell *shell = qobject_cast<Shell*>(calleeData.toQObject());
    int argumentCount = context->argumentCount();

    if (argumentCount > 0) {
        for (int i = 0; i < argumentCount - 1; i++) {
            QScriptValue value = context->argument(i);
            shell->printOut(value, false);
        }
        QScriptValue value = context->argument(argumentCount - 1);
        shell->printOut(value);
    }

    return engine->undefinedValue();
}

/*
 * Class
 */

/*!
    \class Shell
    \mainclass
    \brief Class for create a javascript shell.

    The Shell class allow to create javascript shell in console or with GUI.
*/

/*!
    \fn Shell::Shell(QObject *parent)
    Constructs a Shell with parent object \a parent.
*/
Shell::Shell(QObject *parent) :
    QObject(parent),
    d(new ShellPrivate())
{
    d->engine = new QScriptEngine(this);
}

/*!
    \fn Shell::~Shell()
    Destroy the Shell.
*/
Shell::~Shell() {
    delete d;
}

/*!
    \fn void Shell::printResult(const QScriptValue &result)
    Print an evaluation \a result.

    \bold Note: The default implementation call \l printOut().
*/
void Shell::printResult(const QScriptValue &result) {
    printOut(result);
}

/*!
    \fn void Shell::printUncaughtException(const QScriptValue &exception)
    Print an uncaught \a exception.

    \bold Note: The default implementation call \l printErr().
*/
void Shell::printUncaughtException(const QScriptValue &exception) {
    printErr(exception);
}

/*!
    \fn void Shell::initEngine(QScriptEngine &engine)
    Initialize the script \a engine.
    \sa isEngineInitialized
*/
void Shell::initEngine(QScriptEngine &engine) {
    if (!d->scriptableEngine) {
        d->scriptableEngine = new ScriptableShellEngine(this, this);
    }

    QScriptValue global = engine.globalObject();

    if (d->useGlobalEngine) {
        QScriptValue globalEngine = engine.newQObject(d->scriptableEngine, QScriptEngine::QtOwnership, QScriptEngine::ExcludeChildObjects | QScriptEngine::ExcludeSuperClassContents | QScriptEngine::ExcludeDeleteLater);

        QScriptValueIterator it(global);
        while (it.hasNext()) {
            it.next();
            // we provide our own print implementation, but we want the rest
            if (it.name() != QLatin1String("print")) {
                globalEngine.setProperty(it.name(), it.value(), it.flags());
            }
        }
        global = globalEngine;
        engine.setGlobalObject(globalEngine);
    } else {
        // replace the default print
        QScriptValue printFunction = engine.newFunction(print);
        printFunction.setData(engine.newQObject(this));
        engine.globalObject().setProperty(QString::fromLatin1("print"), printFunction, QScriptValue::Undeletable);
    }

    QScriptValue sr = engine.newObject();
    global.setProperty(QString::fromLatin1("sr"), sr, QScriptValue::Undeletable);

    QScriptValue eng = engine.newQObject(d->scriptableEngine, QScriptEngine::QtOwnership, QScriptEngine::ExcludeChildObjects | QScriptEngine::ExcludeSuperClassContents | QScriptEngine::ExcludeDeleteLater);
    sr.setProperty(QString::fromLatin1("engine"), eng, QScriptValue::Undeletable);
}

/*!
    \fn void Shell::runInteractive()
    Run the shell as interactive mode, when the shell run all available text, sentence by sentence,
    and write the result.
*/
void Shell::runInteractive() {
    if (!d->isInitialized) {
        d->isInitialized = true; // after for prevent an indirect recursive call
        initEngine(*d->engine);
    }
    d->exit = false;
    d->exitCode = 0;

    QString sentence;
    int previousLine = d->lineNumber;
    do {
        sentence = readSentence(d->lineNumber);

        QScriptValue result = d->engine->evaluate(sentence, d->fileName, previousLine);
        previousLine = d->lineNumber;
        if (d->engine->hasUncaughtException()) {
            printUncaughtException(result);
        } else if (!result.isUndefined()) {
            printResult(result);
        }

    } while (!sentence.isNull() && !d->exit);

    if (!d->exit) {
        emit finished(0);
    }
}

/*!
    \fn void Shell::runQuiet()
    Run the shell as interactive quiet mode, when the shell run all available text, sentence by sentence,
    and not write the result.
*/
void Shell::runQuiet() {
    if (!d->isInitialized) {
        d->isInitialized = true; // after for prevent an indirect recursive call
        initEngine(*d->engine);
    }
    d->exit = false;
    d->exitCode = 0;

    QString sentence;
    int previousLine = d->lineNumber;
    do {
        sentence = readSentence(d->lineNumber);

        QScriptValue result = d->engine->evaluate(sentence, d->fileName, previousLine);
        previousLine = d->lineNumber;
        if (d->engine->hasUncaughtException()) {
            printUncaughtException(result);
        }

    } while (!sentence.isNull() && !d->exit);

    if (!d->exit) {
        emit finished(0);
    }
}

/*!
    \fn void Shell::runBatch()
    Run the shell as batch mode, when the shell run all available text and not write the result.
*/
void Shell::runBatch() {
    if (!d->isInitialized) {
        d->isInitialized = true; // after for prevent an indirect recursive call
        initEngine(*d->engine);
    }
    d->exit = false;
    d->exitCode = 0;

    int previousLine = d->lineNumber;
    QString script = readAll(d->lineNumber);

    QScriptValue result = d->engine->evaluate(script, d->fileName, previousLine);
    if (d->engine->hasUncaughtException()) {
        printUncaughtException(result);
    }

    if (!d->exit) {
        emit finished(0);
    }
}

/*!
    \fn void Shell::runOneSentence()
    Run one sentence in the shell, when the shell read a sentence and not write the result.
*/
void Shell::runOneSentence() {
    if (!d->isInitialized) {
        d->isInitialized = true; // after for prevent an indirect recursive call
        initEngine(*d->engine);
    }
    d->exit = false;
    d->exitCode = 0;

    int previousLine = d->lineNumber;
    QString sentence = readSentence(d->lineNumber);

    QScriptValue result = d->engine->evaluate(sentence, d->fileName, previousLine);
    if (d->engine->hasUncaughtException()) {
        printUncaughtException(result);
    }
}

/*!
    \fn void Shell::runOneSentenceInteractive()
    Run one sentence in the shell, when the shell read a sentence and write the result.
*/
void Shell::runOneSentenceInteractive() {
    if (!d->isInitialized) {
        d->isInitialized = true; // after for prevent an indirect recursive call
        initEngine(*d->engine);
    }
    d->exit = false;
    d->exitCode = 0;

    int previousLine = d->lineNumber;
    QString sentence = readSentence(d->lineNumber);

    QScriptValue result = d->engine->evaluate(sentence, d->fileName, previousLine);
    if (d->engine->hasUncaughtException()) {
        printUncaughtException(result);
    } else if (!result.isUndefined()) {
        printResult(result);
    }
}

/*!
    \fn bool Shell::isCompleteSententence(QString sentence)
    Return if \a sentence is a complete sentence.
*/
bool Shell::isCompleteSententence(QString sentence) {
    QScriptSyntaxCheckResult check = QScriptEngine::checkSyntax(sentence);
    return check.state() != QScriptSyntaxCheckResult::Intermediate;
}

/*!
    \fn void Shell::exit(int exitCode)
    Exit the shell with exit code \a exitCode.
    \sa exitCode isExitCalled
*/
void Shell::exit(int exitCode) {
    d->exit = true;
    d->exitCode = exitCode;
    emit finished(exitCode);
}

/*!
    \property Shell::exitCode
    This property specifies the exit code with the shell exit.
    \sa exit() isExitCalled
*/
int Shell::exitCode() const {
    return d->exitCode;
}

/*!
    \fn QString Shell::helpMessage()
    Return the default help message.
*/
QString Shell::helpMessage() {
    QString message = QString::fromLatin1(
            "List of basics commands:\n"
            "    error([message, ...])    print a message in the standard error.\n"
            "    exit()                   exit the shell with return code 0 (zero).\n"
            "    exit(code)               exit the shell with return number code pased as\n"
            "                             the argument 'code'.\n"
            "    getFromEnvironment(name) search for a variable identified by 'name'' and\n"
            "                             returns its value or null if is not found.\n"
            "    help()                   display this help with the basic usage.\n"
            "    importExtension([name, ...]) load the extension(s) with name 'name'.\n"
            "    load([filename, ...])    load and excute the javascrip file(s) with name\n"
            "                             'name' inside the current shell.\n"
            "    print([message, ...])    print a mensagge(s) in the standard output.\n"
            "    quit()                   exit the shell with return code 0 (zero).\n"
            "    read([message, ...])     read a line from the standard input. If a message\n"
            "                             this will be printed an then will read the input.\n"
            "    readFile(name)           read all content of a file.\n"
            "    runCommand(commandName, [arg, ...] [options]) execute the specified command\n"
            "                             with the given argument and options as a separate\n"
            "                             process and return the exit status of the process.\n"
            "                             See runCommand usage for more detail.\n"
            "    runCommand(options)      execute the specified command in the options object\n"
            "                             as a separate process and return the exit status of\n"
            "                             the process. See runCommand usage for more detail.\n"
            "\n"
            "List of basic properties (exposed as a readonly variables):\n"
            "    arguments                list of all argumets given at the command line\n"
            "                             when the shell was invoked.\n"
            "    availableExtensions      list of all avaiable QtScript' extensions.\n"
            "    importedExtensions       list of all imported QtScript' extensions.\n"
            "    qtVersion                version of the Qt library in use.\n"
            "    srVersion                version of the shell, value %1.\n"
            "\n"
            "runCommand() usage:\n"
            "    runCommand(command)\n"
            "    runCommand(command, arg1, ..., argN)\n"
            "    runCommand(command, arg1, ..., argN, options)\n"
            "    runCommand(options)\n"
            "\n"
            "    All except the last arguments to runCommand are converted to strings and\n"
            "    denote command name and its arguments. If the last argument is a JavaScript\n"
            "    object, it is an option object. Otherwise it is converted to string denoting\n"
            "    the last argument and options objects assumed to be empty.\n"
            "\n"
            "    The following properties of the option object are processed:\n"
            "    * args     provides an array of additional command arguments\n"
            "    * env      A JavaScript object with the properties to add to the process\n"
            "               environment. All its enumeratable properties define the\n"
            "               corresponding environment variable names.\n"
            "    * input    the process input. A string sent to the process as its input.\n"
            "               If not specified, no input is provided to the process.\n"
            "    * output   the process output. If it is specified, the process output is\n"
            "               read, converted to a string, appended to the output property\n"
            "               value converted to string and put as the new value of the output\n"
            "               property.\n"
            "    * err      the process error output. If it is specified, the process error\n"
            "               output is read, converted to a string, appended to the err\n"
            "               property value converted to string and put as the new value of\n"
            "               the err property.\n"
            "    * command  command name. Only used if the command name is not passed as\n"
            "               argument of the runCommand method.\n"
            "    * result   exit statud of the process. If it is specified, the exit statud\n"
            "               of the process will be stetted in this property.")
        .arg(QString::fromLatin1(APP_VERSION));
    return message;
}

/*!
    \property Shell::useGlobalEngine
    Specifies if put all members of the \c sr.engine object in the global space.
*/
bool Shell::useGlobalEngine() const {
    return d->useGlobalEngine;
}

void Shell::setUseGlobalEngine(bool useGlobalEngine) {
    d->useGlobalEngine = useGlobalEngine;
}

/*!
    \property Shell::arguments
    \brief Specifies the script arguments.

    This is avaiable in the script with \c sr.engine.arguments
*/
QStringList Shell::arguments() const {
    if (!d->scriptableEngine) {
        return d->scriptableEngine->arguments();
    } else {
        return QStringList();
    }
}

void Shell::setArguments(QStringList arguments) {
    if (!d->scriptableEngine) {
        d->scriptableEngine = new ScriptableShellEngine(this, this);
    }
    d->scriptableEngine->setArguments(arguments);
}


/*!
    \property Shell::fileName
    Specifies the script file name.
*/
QString Shell::fileName() const {
    return d->fileName;
}

void Shell::setFileName(QString fileName) {
    d->fileName = fileName;
}

/*!
    \property Shell::currentLineNumber
    Specifies the line number of the next sentence.
*/
int Shell::currentLineNumber() const {
    return d->lineNumber;
}

void Shell::setCurrentLineNumber(int currentLineNumber) {
    d->lineNumber = currentLineNumber;
}

/*!
    \property Shell::processEventsInterval
    Specifies the script engine process interval. See \bold QScriptEngine::processEventsInterval()
    for more information.
*/
int Shell::processEventsInterval() const {
    return d->engine->processEventsInterval();
}

void Shell::setProcessEventsInterval(int interval) {
    d->engine->setProcessEventsInterval(interval);
}

/*!
    \property Shell::isExitCalled
    Return if \l exit() method is called.
    \sa exit() exitCode
*/
bool Shell::isExitCalled() const {
    return d->exit;
}

/*!
    \property Shell::isEngineInitialized
    Return if the script engine is initialized.
    \sa initEngine()
*/
bool Shell::isEngineInitialized() const {
    return d->isInitialized;
}

/*!
    \fn QScriptEngine* Shell::engine()
    Return the script engine used for run the sentences.
*/
QScriptEngine* Shell::engine() {
    if (!d->isInitialized) {
        d->isInitialized = true; // after for prevent an indirect recursive call
        initEngine(*d->engine);
    }
    return d->engine;
}

/*!
    \fn void Shell::reset()
    Reset the shell.
*/
void Shell::reset() {
    d->lineNumber = 1;
    int interval = d->engine->processEventsInterval();
    delete d->engine;
    d->engine = new QScriptEngine(this);
    d->engine->setProcessEventsInterval(interval);
    d->isInitialized = false;
    d->exit = false;
}

static bool isIdentifierChar(const QChar &ch)
{
    static QChar underscore = QChar::fromLatin1('_');
    return ch.isLetter() || ch.isNumber() || (ch == underscore);
}

static QStringList findCompletions(QScriptContext *context, QStringList path, QString name, QString &commonName) {
    QScriptValue v = context->thisObject();
    int lenght = path.length();

    if (lenght > 0) {
        int pos = 0;
        if (path.at(pos) == QString::fromLatin1("this")) {
            pos++;
        }

        for( ; pos < lenght; pos++) {
            v = v.property(path.at(pos));
        }
    }

    QStringList result;
    if (name.isEmpty()) {
        QScriptValueIterator it(v);
        while (it.hasNext()) {
            it.next();
            result << it.name();
        }
        if (lenght == 0) {
            result << QString::fromLatin1("this");
            result << QString::fromLatin1("true");
            result << QString::fromLatin1("false");
            result << QString::fromLatin1("null");
        }
    } else {

        QString common;
        QScriptValueIterator it(v);
        while (it.hasNext()) {
            it.next();
            QString propertyName = it.name();
            if (propertyName.startsWith(name)) {
                result << propertyName;
                if (common.isNull()) {
                    common = propertyName;
                } else {
                    int i = 0;
                    for( ; i < common.length() && i < propertyName.length(); i++) {
                        if (common.at(i) != propertyName.at(i)) {
                            break;
                        }
                    }
                    common = common.mid(0, i);
                }
            }
        }
        if (!common.isNull()) {
            commonName = common.mid(name.length());
        }
        if (lenght == 0) {
            QString word = QString::fromLatin1("this");
            if (word.startsWith(name)) {
                result << word;
            }
            word = QString::fromLatin1("true");
            if (word.startsWith(name)) {
                result << word;
            }
            word = QString::fromLatin1("false");
            if (word.startsWith(name)) {
                result << word;
            }
            word = QString::fromLatin1("null");
            if (word.startsWith(name)) {
                result << word;
            }
        }
    }

    qStableSort(result);
    return result;
}

/*!
    \fn QStringList Shell::completeScriptExpression(QString expression, int &completitionStartAt, QString &commonName)
    \brief Find completitions of a javascript \a expression.
    Find and return all posible completitions of a javascript \a expression. As output arguments
    \a completitionStartAt contains the positions at the \a expression when the completion posibilities start and
    \a commonName contains the common text between all posibilities of completition.
*/
QStringList Shell::completeScriptExpression(QString expression, int &completitionStartAt, QString &commonName) {
    if (!d->isInitialized) {
        d->isInitialized = true; // after for prevent an indirect recursive call
        initEngine(*d->engine);
    }

    QStringList path;
    QString name;

    if (expression.isEmpty()) {
        completitionStartAt = 0;
        return findCompletions(d->engine->currentContext(), path, name, commonName);
    }

    int endWord = expression.length() - 1;
    int startWord = endWord + 1;
    QChar dot = QChar::fromLatin1('.');

    while (startWord > 0 && isIdentifierChar(expression.at(startWord - 1)) ) {
        startWord--;
    }
    if (startWord <= endWord) {
        name = expression.mid(startWord, endWord - startWord + 1);
    }
    completitionStartAt = startWord;
    startWord--;

    while (startWord > 0 && expression.at(startWord) == dot) {
        endWord = startWord;
        while (startWord > 0 && isIdentifierChar(expression.at(startWord - 1)) ) {
            startWord--;
        }
        QString pathItem = expression.mid(startWord, endWord - startWord);
        path.prepend(pathItem);
        startWord--;
    }

    return findCompletions(d->engine->currentContext(), path, name, commonName);
}

/*!
    \property Shell::version
    Specifies the current shell version
*/
QString Shell::version() const {
    return QString::fromLatin1(APP_VERSION);
}

/*!
   \fn void Shell::finished(int exitCode)
   Signal emmited when \l exit() method is called with \a exitCode.
   \sa exit() exitCode isExitCalled
*/

/*!
    \fn void Shell::helpCommand()
    Show the help. This is called by \c sr.engine.help().
*/

/*!
    \fn QString Shell::readCommand()
    Read a text required by the script. This is called by \c sr.engine.read().
    \sa printForReadCommand()
*/

/*!
    \fn QString Shell::readAll(int &finalLineNumber)
    Read all available text for run as script. As output argument \a finalLineNumber contains
    the line of the next sentence.
*/

/*!
    \fn QString Shell::readSentence(int &finalLineNumber)
    Read one sentence for run as script. As output argument \a finalLineNumber contains
    the line of the next sentence.
*/

/*!
    \fn void Shell::printForReadCommand(const QScriptValue &message, bool last)
    Print a \a message previous to read a text required by the script, the argument \a last indicate
    if it is the last menssage to print previous to read the text. This is called by \c sr.engine.read().
    \sa readCommand()
*/

/*!
    \fn void Shell::printOut(const QScriptValue &message, bool last)
    Print a \a message as output required by the script, the argument \a last indicate
    if is the last message to print. This is called by \c sr.engine.print().
*/

/*!
    \fn void Shell::printErr(const QScriptValue &error, bool last)
    Print a \a error as output required by the script, the argument \a last indicate
    if is the last error to print. This is called by \c sr.engine.error().
*/
