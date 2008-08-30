/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#ifndef _JABBER_XML_H_
#define _JABBER_XML_H_

#define NOID -1

#include <m_xml.h>

void    __fastcall xmlAddChild( HXML, HXML );
HXML    __fastcall xmlAddChild( HXML, LPCTSTR pszName );
HXML    __fastcall xmlAddChild( HXML, LPCTSTR pszName, LPCTSTR ptszValue );
HXML    __fastcall xmlAddChild( HXML, LPCTSTR pszName, int iValue );

LPCTSTR __fastcall xmlGetAttrValue( HXML, LPCTSTR key );
HXML    __fastcall xmlGetChild( HXML, int n = 0 );
HXML    __fastcall xmlGetChild( HXML, LPCSTR key );
HXML    __fastcall xmlGetChild( HXML, LPCTSTR key );
int     __fastcall xmlGetChildCount( HXML );
HXML    __fastcall xmlGetChildByTag( HXML, LPCTSTR key, LPCTSTR attrName, LPCTSTR attrValue );
HXML    __fastcall xmlGetChildByTag( HXML, LPCSTR key, LPCSTR attrName, LPCTSTR attrValue );
HXML    __fastcall xmlGetNthChild( HXML, LPCTSTR key, int n = 0 );

LPCTSTR __fastcall xmlGetName( HXML );
LPCTSTR __fastcall xmlGetText( HXML );

void    __fastcall xmlAddAttr( HXML, LPCTSTR pszName, LPCTSTR ptszValue );
void    __fastcall xmlAddAttr( HXML, LPCTSTR pszName, int value );
void    __fastcall xmlAddAttrID( HXML, int id );

int     __fastcall xmlGetAttrCount( HXML );
LPCTSTR __fastcall xmlGetAttr( HXML, int n );
LPCTSTR __fastcall xmlGetAttrName( HXML, int n );
LPCTSTR __fastcall xmlGetAttrValue( HXML, LPCTSTR key );

struct XmlNode
{
	__forceinline XmlNode() { m_hXml = NULL; }

	__forceinline XmlNode( LPCTSTR pszString, int* numBytes, LPCTSTR ptszTag )
	{
		m_hXml = xi.parseString( pszString, numBytes, ptszTag );
	}

	XmlNode( const XmlNode& n );
	XmlNode( LPCTSTR name );
	XmlNode( LPCTSTR pszName, LPCTSTR ptszText );
	~XmlNode();

	XmlNode& operator =( const XmlNode& n );

	__forceinline operator HXML() const
	{	return m_hXml;
	}

private:
	HXML m_hXml;
};

class CJabberIqInfo;

struct XmlNodeIq : public XmlNode
{
	XmlNodeIq( const TCHAR* type, int id = -1, const TCHAR* to = NULL );
	XmlNodeIq( const TCHAR* type, const TCHAR* idStr, const TCHAR* to );
	XmlNodeIq( const TCHAR* type, HXML node, const TCHAR* to );
	// new request
	XmlNodeIq( CJabberIqInfo* pInfo );
	// answer to request
	XmlNodeIq( const TCHAR* type, CJabberIqInfo* pInfo );
};

typedef void ( *JABBER_XML_CALLBACK )( HXML, void* );

/////////////////////////////////////////////////////////////////////////////////////////

struct XATTR
{
	LPCTSTR name, value;

	__forceinline XATTR( LPCTSTR _name, LPCTSTR _value ) :
		name( _name ),
		value( _value )
		{}
};

HXML __forceinline operator<<( HXML node, XATTR& attr )
{	xmlAddAttr( node, attr.name, attr.value );
	return node;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct XATTRI
{
	LPCTSTR name;
	int value;

	__forceinline XATTRI( LPCTSTR _name, int _value ) :
		name( _name ),
		value( _value )
		{}
};

HXML __forceinline operator<<( HXML node, XATTRI& attr )
{	xmlAddAttr( node, attr.name, attr.value );
	return node;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct XATTRID
{
	int id;

	__forceinline XATTRID( int _value ) :
		id( _value )
		{}
};

HXML __forceinline operator<<( HXML node, XATTRID& attr )
{	xmlAddAttrID( node, attr.id );
	return node;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct XCHILD
{
	LPCTSTR name, value;

	__forceinline XCHILD( LPCTSTR _name, LPCTSTR _value = NULL ) :
		name( _name ),
		value( _value )
		{}
};

HXML __forceinline operator<<( HXML node, XCHILD& child )
{	return xmlAddChild( node, child.name, child.value );
}

/////////////////////////////////////////////////////////////////////////////////////////

struct XCHILDNS
{
	LPCTSTR name, ns;

	__forceinline XCHILDNS( LPCTSTR _name, LPCTSTR _ns = NULL ) :
		name( _name ),
		ns( _ns )
		{}
};

HXML __fastcall operator<<( HXML node, XCHILDNS& child );

/////////////////////////////////////////////////////////////////////////////////////////

struct XQUERY
{
	LPCTSTR ns;

	__forceinline XQUERY( LPCTSTR _ns ) :
		ns( _ns )
		{}
};

HXML __fastcall operator<<( HXML node, XQUERY& child );

/////////////////////////////////////////////////////////////////////////////////////////
// Limited XPath support
//     path should look like: "node-spec/node-spec/.../result-spec"
//     where "node-spec" can be "node-name", "node-name[@attr-name='attr-value']" or "node-name[node-index]"
//     result may be either "node-spec", or "@attr-name"
//
// Samples:
//    LPCTSTR s = XPathT(node, "child/subchild[@attr='value']");          // get node text
//    LPCTSTR s = XPathT(node, "child/subchild[2]/@attr");                // get attribute value
//    XPathT(node, "child/subchild[@name='test']/@attr") = _T("Hello");   // create path if needed and set attribute value
//
//    XPathT(node, "child/subchild[@name='test']") = _T("Hello");         // TODO: create path if needed and set node text

#define XPathT(a,b) XPath(a, _T(b))

class XPath
{
public:
	XPath(HXML hXml, TCHAR *path):
		m_type(T_UNKNOWN),
		m_hXml(hXml),
		m_szPath(path),
		m_szParam(NULL)
		{}

	// Read data
	__forceinline operator HXML()
	{
		switch (Lookup())
		{
			case T_NODE: return m_hXml;
			case T_NODESET: return xmlGetNthChild(m_hXml, m_szParam, 1);
		}
		return NULL;
	}
	__forceinline operator LPCTSTR()
	{
		switch (Lookup())
		{
			case T_ATTRIBUTE: return xmlGetAttrValue(m_hXml, m_szParam);
			case T_NODE: return xmlGetText(m_hXml);
			case T_NODESET: return xmlGetText(xmlGetNthChild(m_hXml, m_szParam, 1));
		}
		return NULL;
	}
	__forceinline HXML operator[] (int idx)
	{
		return (Lookup() == T_NODESET) ? xmlGetNthChild(m_hXml, m_szParam, idx) : NULL;
	}

	// Write data
	__forceinline void operator= (LPCTSTR value)
	{
		switch (Lookup(true))
		{
			case T_ATTRIBUTE: xmlAddAttr(m_hXml, m_szParam, value); break;
			case T_NODE: break; // TODO: set node text
		}
	}

private:
	enum PathType
	{
		T_UNKNOWN,
		T_ERROR,
		T_NODE,
		T_ATTRIBUTE,
		T_NODESET
	};

	__forceinline PathType Lookup(bool bCreate=false)
	{
		return (m_type == T_UNKNOWN) ? LookupImpl(bCreate) : m_type;
	}

	enum LookupState
	{
		S_START,
		S_ATTR_STEP,
		S_NODE_NAME,
		S_NODE_OPENBRACKET,
		S_NODE_INDEX,
		S_NODE_ATTRNAME,
		S_NODE_ATTREQUALS,
		S_NODE_ATTRVALUE,
		S_NODE_ATTRCLOSEVALUE,
		S_NODE_CLOSEBRACKET,

		S_FINAL,
		S_FINAL_ERROR,
		S_FINAL_ATTR,
		S_FINAL_NODESET,
		S_FINAL_NODE
	};

	struct LookupString
	{
		void Begin(const TCHAR *p_) { p = p_; }
		void End(const TCHAR *p_) { length = p_ - p; }
		operator bool() { return p ? true : false; }

		const TCHAR *p;
		int length;

	};

	struct LookupInfo
	{
		void Reset() { memset(this, 0, sizeof(*this)); }
		LookupString nodeName;
		LookupString attrName;
		LookupString attrValue;
		LookupString nodeIndex;
	};

	void ProcessPath(LookupInfo &info, bool bCreate);
	PathType LookupImpl(bool bCreate);

	PathType m_type;
	HXML m_hXml;
	LPCTSTR m_szPath;
	LPCTSTR m_szParam;
};

#endif
