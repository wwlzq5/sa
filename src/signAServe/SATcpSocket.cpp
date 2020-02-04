#include "SATcpSocket.h"
#include "SAProtocolHeader.h"
#include "SAXMLProtocolParser.h"
#include "SATcpXMLSocketDelegate.h"
class SATcpSocketPrivate
{
    SA_IMPL_PUBLIC(SATcpSocket)
public:
    SATcpSocketPrivate(SATcpSocket* p);
    void reset();
    void mallocBuffer(size_t size);
    SAProtocolHeader m_mainHeader;
    bool m_isReadedMainHeader;
    uint m_dataSize;
    uint m_index;
    QByteArray m_buffer;
    SATcpSocketDelegate* m_delegate;
};

SATcpSocketPrivate::SATcpSocketPrivate(SATcpSocket *p):q_ptr(p)
  ,m_isReadedMainHeader(false)
  ,m_dataSize(0)
  ,m_index(0)
  ,m_delegate(new SATcpXMLSocketDelegate(p))
{

}

void SATcpSocketPrivate::reset()
{
    m_isReadedMainHeader = false;
    m_dataSize = 0;
    m_index = 0;
}

void SATcpSocketPrivate::mallocBuffer(size_t size)
{
    m_buffer.resize(size);
    m_index = 0;
    m_dataSize = size;
}

SATcpSocket::SATcpSocket(QObject *par)
{
    connect(this,&QIODevice::readyRead,this,&SATcpSocket::onReadyRead);
}

SATcpSocket::~SATcpSocket()
{

}

/**
 * @brief 从socket中安全的读取文件
 * @param p 指针
 * @param n 读取的长度
 * @return
 */
bool SATcpSocket::readFromSocket(void *p, int n)
{
    int readLen = 0,index = 0;
    bool ret = false;
    do
    {
        readLen = read((char*)p+index,n);
        if(readLen <= 0)
        {
            break;
        }
        index += readLen;
        n -= readLen;
    }while( readLen && n );
    if(0 == n)
    {
        ret = true;
    }
    return ret;
}

SATcpSocketDelegate *SATcpSocket::getDelegate() const
{
    return d_ptr->m_delegate;
}

/**
 * @brief 设置新代理，代理会自动把parent设定为satcpsocket
 * @param delegate
 * @note 旧的代理会被销毁，用户不需要手动销毁
 */
void SATcpSocket::setupDelegate(SATcpSocketDelegate *delegate)
{
    SATcpSocketDelegate* old = d_ptr->m_delegate;
    if(old == delegate)
    {
        return;
    }
    if(nullptr != old)
    {
        delete old;
    }
    if(delegate->parent() != this)
    {
        delegate->setParent(this);
    }
    d_ptr->m_delegate = delegate;
}

void SATcpSocket::deal(const SAProtocolHeader &header, const QByteArray &data)
{
    d_ptr->m_delegate->deal(header,data);
    emit receivedData(header, data);
}

void SATcpSocket::onReadyRead()
{
    const static unsigned int s_headerSize = sizeof(SAProtocolHeader);

    if(d_ptr->m_isReadedMainHeader)
    {
        //此时已经读取了头
        if(bytesAvailable() >= d_ptr->m_mainHeader.dataSize)
        {
            //说明数据接收完
#ifdef _DEBUG_PRINT
            qDebug() << " rec Data:"<<bytesAvailable()<< " bytes "
                     <<"\n header:"<<d_ptr->m_mainHeader
                        ;
#endif
            if(d_ptr->m_buffer.size() < d_ptr->m_mainHeader.dataSize)
            {
                d_ptr->m_buffer.resize(d_ptr->m_mainHeader.dataSize);
            }
            if(!readFromSocket(d_ptr->m_buffer.data()+d_ptr->m_index,d_ptr->m_mainHeader.dataSize-d_ptr->m_index))
            {
                d_ptr->reset();
                abort();
                qDebug() << "socket abort!!!  can not read from socket io!";
                return;
            }
#ifdef _DEBUG_PRINT
            printQByteArray(d_ptr->m_buffer);
#endif
            deal(d_ptr->m_mainHeader,d_ptr->m_buffer);
            d_ptr->reset();
            if(bytesAvailable() >= s_headerSize)
            {
                onReadyRead();
            }
        }
        else
        {
            //说明一次没把数据接收完

        }
    }
    else if(bytesAvailable() >= s_headerSize)
    {
        //说明包头还未接收
        //但socket收到的数据已经满足包头数据需要的数据
#ifdef _DEBUG_PRINT
        qDebug() <<"main header may receive:"
                << "\r\n byte available:"<<bytesAvailable()
                    ;
#endif
        if(!readFromSocket((void*)(&(d_ptr->m_mainHeader)),s_headerSize))
        {
            qDebug() << "can not read from socket" << __LINE__;
            d_ptr->m_isReadedMainHeader = false;
            abort();
            return;
        }
#ifdef _DEBUG_PRINT
        qDebug() << "readed header from socket:"
                 << d_ptr->m_mainHeader
                 ;
#endif
        d_ptr->m_index = 0; // 不需要mallocBuffer会设置此值
        if(!(d_ptr->m_mainHeader.isValid()))
        {
            d_ptr->reset();
            qDebug() << "receive unknow header[1]"
                     << "\n" << d_ptr->m_mainHeader;
            abort();
            return;
        }

        if(d_ptr->m_mainHeader.dataSize > 0)
        {
            //说明文件头之后有数据
            d_ptr->m_isReadedMainHeader = true;
            //分配好内存
            d_ptr->mallocBuffer(d_ptr->m_mainHeader.dataSize);
        }
        else
        {
            //说明文件头之后无数据
            deal(d_ptr->m_mainHeader,QByteArray());
            d_ptr->reset();
            d_ptr->m_isReadedMainHeader = false;
        }

        if(bytesAvailable() > 0)
        {
#ifdef _DEBUG_PRINT
        qDebug() << "have avaliable data,size:"<<bytesAvailable()
                        ;
#endif
            onReadyRead();
        }
    }
}



