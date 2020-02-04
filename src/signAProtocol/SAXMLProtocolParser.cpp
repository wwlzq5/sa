#include "SAXMLProtocolParser.h"
#include <QDomDocument>
#include <QHash>
#include <QDomNodeList>
#include "SAXMLTagDefined.h"
#include "SAVariantCaster.h"

#define DEFAULT_GROUP_NAME "${sa-default-group}"

class SAXMLProtocolParserPrivate
{
    SA_IMPL_PUBLIC(SAXMLProtocolParser)
public:
    SAXMLProtocolParserPrivate(SAXMLProtocolParser* p);
    ~SAXMLProtocolParserPrivate();
    bool isValid() const;
    bool fromByteArray(const QByteArray &data);
    bool fromString(const QString &str);
    bool isHasGroup(const QString &groupName) const;
    bool isHasKey(const QString &groupName, const QString &keyName) const;
    QString makeFullItemName(const QString &groupName, const QString &keyName) const;
    QDomElement getGroupEle(const QString &groupName) const;
    //有别于getGroupEle，如果没有这个组会创建一个组，此函数必定返回一个有效的组节点
    QDomElement testGroupEle(const QString &groupName);
    QVariant getValue(const QString &keyName, const QVariant &defaultVal = QVariant()) const;
    QVariant getValue(const QString &groupName, const QString &keyName, const QVariant &defaultVal = QVariant()) const;
    QVariant getValue(QDomElement item, const QVariant &defaultVal = QVariant()) const;
    QDomElement getValueEle(const QString &groupName, const QString &keyName) const;
    //写入value数据
    QDomElement setValue(const QString &keyName, const QVariant &var);
    QDomElement setValue(const QString &groupName, const QString &keyName, const QVariant &var);
    //写入value数据，数据的父级对象不在mValuesEle下，而是指定在par下
    QDomElement setValue(const QString &keyName, const QVariant &var, QDomElement par);
    //获取组写的所有item名
    QStringList getKeyNames(const QString &groupName) const;
    //
    void setClassID(int classid);
    void setFunID(int funid);
    //清空内容
    void _clear();
    int mClassID;
    int mFunID;
    QDomDocument mDoc;
    QString mErrorMsg;// 错误信息
    QDomElement mRootEle; //根节点
    QDomElement mValuesEle; //根节点
    QDomElement mDefaultGroup;// 默认分组
    QHash<QString,QDomElement> mGroupEle; //保存所有的组节点
    QHash<QString,QDomElement> mItemEles; // 保存所有的item节点，key是GroupName/ItemName的组合
    bool mIsValid;
private:
    bool parserDoc();
};

SAXMLProtocolParserPrivate::SAXMLProtocolParserPrivate(SAXMLProtocolParser *p)
    :q_ptr(p)
    ,mClassID(-1)
    ,mFunID(-1)
    ,mIsValid(false)
{
    mRootEle = mDoc.createElement(SA_XML_TAG_SA);
    mRootEle.setAttribute(SA_XML_ATT_TYPE,"xml");
    mRootEle.setAttribute(SA_XML_ATT_NAME_CLASSID,mClassID);
    mRootEle.setAttribute(SA_XML_ATT_NAME_FUNID,mFunID);
    mDoc.appendChild(mRootEle);
    mValuesEle = mDoc.createElement(SA_XML_TAG_VALUES);
    mRootEle.appendChild(mValuesEle);
    mDefaultGroup = mDoc.createElement(SA_XML_TAG_DEFAULTGROUP);
    mValuesEle.appendChild(mDefaultGroup);
}

SAXMLProtocolParserPrivate::~SAXMLProtocolParserPrivate()
{

}

bool SAXMLProtocolParserPrivate::isValid() const
{
    return mIsValid;
}

bool SAXMLProtocolParserPrivate::fromByteArray(const QByteArray &data)
{
    _clear();
    bool isok = mDoc.setContent(data,&mErrorMsg);
    if (!isok)
        return false;
    return parserDoc();
}

bool SAXMLProtocolParserPrivate::fromString(const QString &str)
{
    _clear();
    bool isok = mDoc.setContent(str,&mErrorMsg);
    if (!isok)
        return false;
    return parserDoc();
}


bool SAXMLProtocolParserPrivate::parserDoc()
{
    // 获取根节点
    mRootEle = mDoc.documentElement();
    if (mRootEle.isNull())
    {
        mErrorMsg = QObject::tr("root element error");
        return false;
    }
    // 到这里就认为协议正确，无论后面有没有内容
    mIsValid = true;
    //获取类号和功能号
    mClassID = mRootEle.attribute(SA_XML_ATT_NAME_CLASSID,"0").toInt();
    mFunID = mRootEle.attribute(SA_XML_ATT_NAME_FUNID,"0").toInt();
    // 获取values节点,如果有多个values，会忽略后面的values
    QDomNodeList valuesEle = mRootEle.elementsByTagName(SA_XML_TAG_VALUES);
    if (valuesEle.isEmpty())
    {
        return mIsValid;
    }
    mValuesEle = valuesEle.at(0).toElement();
    // 获取default group
    QDomNodeList defaultgroupEles = mValuesEle.elementsByTagName(SA_XML_TAG_DEFAULTGROUP);
    auto size = defaultgroupEles.size();
    for (auto i = 0;i<size;++i)
    {
        QDomElement ge = defaultgroupEles.at(i).toElement();
        // 获取default group下的item信息
        QDomNodeList itemEles = ge.elementsByTagName(SA_XML_TAG_ITEM);
        auto itemsize = itemEles.size();
        for (auto j = 0;j<itemsize;++j)
        {
            QDomElement ie = itemEles.at(j).toElement();
            QString itemname = ie.attribute(SA_XML_ATT_NAME);
            mItemEles[makeFullItemName(DEFAULT_GROUP_NAME,itemname)] = ie;
        }
    }
    // 获取group内容
    QDomNodeList groupEles = mValuesEle.elementsByTagName(SA_XML_TAG_GROUP);
    size = groupEles.size();
    for (auto i = 0;i<size;++i)
    {
        QDomElement ge = groupEles.at(i).toElement();
        QString groupname = ge.attribute(SA_XML_ATT_NAME);
        if (groupname.isEmpty())
            continue;
        mGroupEle[groupname] = ge;
        // 获取group下的item信息
        QDomNodeList itemEles = ge.elementsByTagName(SA_XML_TAG_ITEM);
        auto itemsize = itemEles.size();
        for (auto j = 0;j<itemsize;++j)
        {
            QDomElement ie = itemEles.at(j).toElement();
            QString itemname = ie.attribute(SA_XML_ATT_NAME);
            mItemEles[makeFullItemName(groupname,itemname)] = ie;
        }
    }
    return mIsValid;
}

bool SAXMLProtocolParserPrivate::isHasGroup(const QString &groupName) const
{
    return mGroupEle.contains(groupName);
}

bool SAXMLProtocolParserPrivate::isHasKey(const QString &groupName, const QString &keyName) const
{
    return mItemEles.contains(makeFullItemName(groupName,keyName));
}

QString SAXMLProtocolParserPrivate::makeFullItemName(const QString &groupName, const QString &keyName) const
{
    return groupName + "/" + keyName;
}

QDomElement SAXMLProtocolParserPrivate::getGroupEle(const QString &groupName) const
{
    return mGroupEle.value(groupName,QDomElement());
}

QDomElement SAXMLProtocolParserPrivate::testGroupEle(const QString &groupName)
{
    QDomElement g = mGroupEle.value(groupName,QDomElement());
    if (g.isNull())
    {
        //没有发现对应名字分组，创建一个
        g = mDoc.createElement(groupName);
        g.setAttribute(SA_XML_ATT_NAME,groupName);
        mGroupEle[groupName] = g;
    }
    return g;
}

QVariant SAXMLProtocolParserPrivate::getValue(const QString &keyName, const QVariant &defaultVal) const
{
    QDomElement item = mItemEles.value(makeFullItemName(DEFAULT_GROUP_NAME,keyName),QDomElement());
    return getValue(item,defaultVal);
}

QVariant SAXMLProtocolParserPrivate::getValue(const QString &groupName, const QString &keyName, const QVariant &defaultVal) const
{
    QDomElement item = getValueEle(groupName,keyName);
    return getValue(item,defaultVal);
}

QVariant SAXMLProtocolParserPrivate::getValue(QDomElement item, const QVariant &defaultVal) const
{
    if (item.isNull())
        return defaultVal;
    //

    QString vartype = item.attribute(SA_XML_ATT_TYPE);
    if(0 == QString::compare(vartype,SA_XML_VAR_ARR_LIST))
    {
        QList<QVariant> res;
        //获取子节点数
        QDomNodeList ci = item.elementsByTagName(SA_XML_TAG_ITEM);
        auto size = ci.size();
        for (auto i = 0;i<size;++i)
        {
            QVariant v = getValue(ci.at(i).toElement(),QVariant());
            if(v.isValid())
            {
                res.append(v);
            }
        }
        return QVariant(res);
    }
    else if(0 == QString::compare(vartype,SA_XML_VAR_ARR_STRLIST,Qt::CaseInsensitive))
    {
        QStringList res;
        //获取子节点数
        QDomNodeList ci = item.elementsByTagName(SA_XML_TAG_ITEM);
        auto size = ci.size();
        for (auto i = 0;i<size;++i)
        {
            QVariant v = getValue(ci.at(i).toElement(),QVariant());
            if(v.isValid())
            {
                QString s = v.toString();
                res.append(s);
            }
        }
        return QVariant(res);
    }
    else if(0 == QString::compare(vartype,SA_XML_VAR_ARR_MAP,Qt::CaseInsensitive))
    {
        QMap<QString, QVariant> res;
        //获取子节点数
        QDomNodeList ci = item.elementsByTagName(SA_XML_TAG_ITEM);
        auto size = ci.size();
        for (auto i = 0;i<size;++i)
        {
            QDomElement childitem = ci.at(i).toElement();
            QString itemkeyname = childitem.attribute(SA_XML_ATT_NAME);
            QVariant v = getValue(childitem,QVariant());
            res[itemkeyname] = v;
        }
        return QVariant(res);
    }
    else if(0 == QString::compare(vartype,SA_XML_VAR_ARR_HASH,Qt::CaseInsensitive))
    {
        QHash<QString, QVariant> res;
        //获取子节点数
        QDomNodeList ci = item.elementsByTagName(SA_XML_TAG_ITEM);
        auto size = ci.size();
        for (auto i = 0;i<size;++i)
        {
            QDomElement childitem = ci.at(i).toElement();
            QString itemkeyname = childitem.attribute(SA_XML_ATT_NAME);
            QVariant v = getValue(childitem,QVariant());
            res[itemkeyname] = v;
        }
        return QVariant(res);
    }
    QString varstr = item.nodeValue();
    return SAVariantCaster::stringToVariant(varstr,vartype);
}

QDomElement SAXMLProtocolParserPrivate::getValueEle(const QString &groupName, const QString &keyName) const
{
    return mItemEles.value(makeFullItemName(groupName,keyName),QDomElement());
}

QDomElement SAXMLProtocolParserPrivate::setValue(const QString &keyName, const QVariant &var)
{
    QDomElement i = setValue(keyName,var,mDefaultGroup);
    mItemEles[makeFullItemName(DEFAULT_GROUP_NAME,keyName)] = i;
    return i;
}

QDomElement SAXMLProtocolParserPrivate::setValue(const QString &groupName, const QString &keyName, const QVariant &var)
{
    QDomElement g = testGroupEle(groupName);
    QDomElement i = setValue(keyName,var,g);
    mItemEles[makeFullItemName(groupName,keyName)] = i;
    return i;
}

QDomElement SAXMLProtocolParserPrivate::setValue(const QString &keyName, const QVariant &var, QDomElement par)
{
    QDomElement item = mDoc.createElement(SA_XML_TAG_ITEM);
    QString vartype = var.typeName();
    item.setAttribute(SA_XML_ATT_NAME,keyName);
    item.setAttribute(SA_XML_ATT_TYPE,vartype);
    //对数组类型进行特殊处理
    if(0 == QString::compare(vartype,SA_XML_VAR_ARR_LIST,Qt::CaseInsensitive))
    {
        QList<QVariant> l = var.toList();
        for (auto i = l.begin();i!=l.end();++i)
        {
            setValue("",*i,item);
        }
    }
    else if (0 == QString::compare(vartype,SA_XML_VAR_ARR_MAP,Qt::CaseInsensitive))
    {
        QMap<QString, QVariant> l = var.toMap();
        for (auto i = l.begin();i!=l.end();++i)
        {
            setValue(i.key(),i.value(),item);
        }
    }
    else if (0 == QString::compare(vartype,SA_XML_VAR_ARR_HASH,Qt::CaseInsensitive))
    {
        QHash<QString, QVariant> l = var.toHash();
        for (auto i = l.begin();i!=l.end();++i)
        {
            setValue(i.key(),i.value(),item);
        }
    }
    else if (0 == QString::compare(vartype,SA_XML_VAR_ARR_STRLIST,Qt::CaseInsensitive))
    {
        QStringList l = var.toStringList();
        for (auto i = l.begin();i!=l.end();++i)
        {
            setValue("",*i,item);
        }
    }
    else
    {
        item.setNodeValue(SAVariantCaster::variantToString(var));
    }
    par.appendChild(item);
    return item;
}

QStringList SAXMLProtocolParserPrivate::getKeyNames(const QString &groupName) const
{
    QStringList res;
    QDomElement g = getGroupEle(groupName);
    if (g.isNull())
        return res;
    QDomNodeList items = g.elementsByTagName(SA_XML_TAG_ITEM);
    auto size = items.size();
    for (int i=0;i<size;++i)
    {
        QDomElement ie = items.at(i).toElement();
        if(ie.isNull())
        {
            continue;
        }
        QString n = ie.attribute(SA_XML_ATT_NAME);
        if(!n.isEmpty())
            res.append(n);
    }
    return res;
}

void SAXMLProtocolParserPrivate::setClassID(int classid)
{
    QDomAttr n = mRootEle.attributeNode(SA_XML_ATT_NAME_CLASSID);
    if(n.isNull())
    {
        n = mDoc.createAttribute(SA_XML_ATT_NAME_CLASSID);
        mRootEle.setAttributeNode(n);
    }
    n.setValue(QString::number(classid));
}

void SAXMLProtocolParserPrivate::setFunID(int funid)
{
    QDomAttr n = mRootEle.attributeNode(SA_XML_ATT_NAME_FUNID);
    if(n.isNull())
    {
        n = mDoc.createAttribute(SA_XML_ATT_NAME_FUNID);
        mRootEle.setAttributeNode(n);
    }
    n.setValue(QString::number(funid));
}


void SAXMLProtocolParserPrivate::_clear()
{
    mGroupEle.clear();
    mItemEles.clear();
    mIsValid = false;
    mDoc.clear();
    mErrorMsg = "";
    mRootEle = QDomElement();
    mValuesEle = QDomElement();
}


SAXMLProtocolParser::SAXMLProtocolParser(QObject *par):SAAbstractProtocolParser(par)
  ,d_ptr(new SAXMLProtocolParserPrivate(this))
{

}

SAXMLProtocolParser::~SAXMLProtocolParser()
{

}

void SAXMLProtocolParser::setFunctionID(int funid)
{
    d_ptr->setFunID(funid);
}

int SAXMLProtocolParser::getFunctionID() const
{
    return d_ptr->mFunID;
}

void SAXMLProtocolParser::setClassID(int classid)
{
    d_ptr->setClassID(classid);
}

int SAXMLProtocolParser::getClassID() const
{
    return d_ptr->mClassID;
}

void SAXMLProtocolParser::setValue(const QString &groupName, const QString &keyName, const QVariant &var)
{
    d_ptr->setValue(groupName,keyName,var);
}

void SAXMLProtocolParser::setValue(const QString &keyName, const QVariant &var)
{
    d_ptr->setValue(keyName,var);
}

QStringList SAXMLProtocolParser::getGroupNames() const
{
    return d_ptr->mGroupEle.keys();
}

QStringList SAXMLProtocolParser::getKeyNames(const QString &groupName) const
{
    return d_ptr->getKeyNames(groupName);
}

bool SAXMLProtocolParser::fromString(const QString &str)
{
    return d_ptr->fromString(str);
}

QString SAXMLProtocolParser::toString() const
{
    return d_ptr->mDoc.toString();
}

bool SAXMLProtocolParser::fromByteArray(const QByteArray &data)
{
    return d_ptr->fromByteArray(data);
}

QByteArray SAXMLProtocolParser::toByteArray() const
{
    return toString().toUtf8();
}

bool SAXMLProtocolParser::isHasGroup(const QString &groupName) const
{
    return d_ptr->isHasGroup(groupName);
}

bool SAXMLProtocolParser::isHasKey(const QString &groupName, const QString &keyName) const
{
    return d_ptr->isHasKey(groupName,keyName);
}

QVariant SAXMLProtocolParser::getValue(const QString &groupName, const QString &keyName, const QVariant &defaultVal) const
{
    return d_ptr->getValue(groupName,keyName,defaultVal);
}

QVariant SAXMLProtocolParser::getValue(const QString &keyName, const QVariant &defaultVal) const
{
    return d_ptr->getValue(keyName,defaultVal);
}

/**
 * @brief 获取错误信息
 * @return 在@sa setProtocolData 返回false时调用
 */
QString SAXMLProtocolParser::getErrorString() const
{
    return d_ptr->mErrorMsg;
}


