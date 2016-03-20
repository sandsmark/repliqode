#include "streamredirector.h"

StreamRedirector::StreamRedirector(std::ostream &stream) :
    m_stream(stream)
{
    m_old_buf = stream.rdbuf();
    stream.rdbuf(this);
}

StreamRedirector::~StreamRedirector()
{
    m_stream.rdbuf(m_old_buf);
}

std::streambuf::int_type StreamRedirector::overflow(std::streambuf::int_type v)
{
    if (v == '\n') {
        emit stringOutput(QStringLiteral("\n"));
    }

    return v;
}

std::streamsize StreamRedirector::xsputn(const char *p, std::streamsize n)
{
    emit stringOutput(QString::fromLocal8Bit(p, n));
    return n;
}
