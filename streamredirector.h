#ifndef STREAMREDIRECTOR_H
#define STREAMREDIRECTOR_H

#include <QObject>

#include <iostream>
#include <streambuf>
#include <string>

class StreamRedirector : public QObject, public std::basic_streambuf<char>
{
    Q_OBJECT
public:
    StreamRedirector(std::ostream &stream);

    virtual ~StreamRedirector();

signals:
    void stringOutput(QString string);

protected:
    virtual int_type overflow(int_type v) override;

    virtual std::streamsize xsputn(const char *p, std::streamsize n) override;

private:
    std::ostream &m_stream;
    std::streambuf *m_old_buf;
};

#endif // STREAMREDIRECTOR_H
