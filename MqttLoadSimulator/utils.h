#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QCommandLineParser>
#include <type_traits>

class ArgumentException : public std::runtime_error
{
public:
    ArgumentException(const std::string &s);
};

QString GetRandomString();
void seedQtrand();
std::string formatString(const std::string str, ...);

template<class T>
typename std::enable_if<std::is_signed<T>::value, T>::type
parseIntOption(QCommandLineParser &parser, QCommandLineOption &option)
{
    bool parsed = false;

    T val = parser.value(option).toInt(&parsed);
    if (!parsed)
    {
        throw ArgumentException(formatString("Option %s is not an int", qPrintable(option.names().first())));
    }
    return val;
}

template<class T>
typename std::enable_if<std::is_unsigned<T>::value, T>::type
parseIntOption(QCommandLineParser &parser, QCommandLineOption &option)
{
    bool parsed = false;

    T val = parser.value(option).toUInt(&parsed);
    if (!parsed)
    {
        throw ArgumentException(formatString("Option %s is not an uint", qPrintable(option.names().first())));
    }
    return val;
}

#endif // UTILS_H
