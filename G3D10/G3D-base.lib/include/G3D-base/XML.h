/**
  \file G3D-base.lib/include/G3D-base/XML.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#pragma once

#include "G3D-base/platform.h"
#include "G3D-base/Table.h"
#include "G3D-base/Array.h"
#include "G3D-base/format.h"
#include "G3D-base/G3DString.h"

namespace G3D {

class TextInput;
class TextOutput;

/** 
\brief Easy loading and saving of XML and HTML files.

The XML class is intended primarily for interchange with other
programs. We recommend using G3D::Any to make your own human-readable
formats because it is a more general syntax, the implementation is
more efficient, and contains better error handling.

Every XML is either a <i>VALUE</i>, or a <i>TAG</i> that contains both
a table of its XML attributes and an array of its children. Children
are nested tags and the strings between the nested tags.

No validation is performed, and the XML must be completely legal.  XML
Entity references (e.g., the ampersand codes for greater than and less
than) are not automatically converted.

Tags with names that begin with "!" or "?" are ignored. Comment tags must
end with "--&gt;" e.g.,

<pre>
  &lt;?xml version="1.0" encoding="ISO-8859-1"?&gt;
  &lt;!DOCTYPE note SYSTEM "Note.dtd"&gt;
  &lt;!-- a comment --&gt;
</pre>

\sa G3D::Any, http://www.grinninglizard.com/tinyxml/

\htmlonly
<pre>
&lt;foo key0="value0" key1="value1"&gt;
  child0 ...
  &lt;child1&gt;...</child1&gt;
  child2 ...
&lt;/foo&gt;
</pre>
\endhtmlonly
*/
class XML {
public:

    enum Type {VALUE, TAG};

    typedef Table<String, XML> AttributeTable;

private:

    Type                      m_type;
    String                    m_name;
    String                    m_value;
    AttributeTable            m_attribute;
    Array<XML>                m_child;

public:

    XML() : m_type(VALUE) {}

    XML(const String& v) : m_type(VALUE), m_value(v) {}

    XML(const double& v) : m_type(VALUE), m_value(format("%f", v)) {}

    XML(float v) : m_type(VALUE), m_value(format("%f", v)) {}

    XML(int v) : m_type(VALUE), m_value(format("%d", v)) {}

    /** \param tagType Must be XML::TAG to dismbiguate from the string constructor */
    XML(Type tagType, const String& name, const AttributeTable& at, const Array<XML>& ch = Array<XML>()) : m_type(TAG), m_name(name), m_attribute(at), m_child(ch) {
        (void)tagType;
        debugAssert(tagType == TAG);
    }

    /** \param tagType Must be XML::TAG to dismbiguate from the string constructor */
    XML(Type tagType, const String& name, const Array<XML>& ch = Array<XML>()) : m_type(TAG), m_name(name), m_child(ch) {
        (void)tagType;
        debugAssert(tagType == TAG);
    }

    XML(TextInput& t);

    void serialize(TextOutput& t, bool collapseEmptyTags = false) const;

    void deserialize(TextInput& t);

    void load(const String& filename);

    void save(const String& filename, bool collapseEmptyTags = false) const;

    void parse(const String &s);

    /** 
        If collapseEmptyTags, writes tags with no children as a single tag
    
        For example: <code>&lt;name atr0="val0"&gt;&lt;/name&gt;</code>
        Is instead: <code>&lt;name atr0="val0"/&gt;</code>
    */
    void unparse(String& s, bool collapseEmptyTags = false) const;

    const AttributeTable& attributeTable() const {
        return m_attribute;
    }

    const Array<XML>& childArray() const {
        return m_child;
    }
    
    /** Array size; zero for a VALUE */
    int numChildren() const {
        return m_child.size();
    }

    /** Attribute table size; zero for a TAG */
    size_t numAttributes() const {
        return m_attribute.size();
    }

    /** Return child \a i.  Children are nested tags and the unquoted
     strings of characters between tags.*/
    const XML& operator[](int i) const {
        return m_child[i];
    }

    /** Return the attribute with this name. */
    const XML& operator[](const String& k) const {
        return m_attribute[k];
    }

    bool containsAttribute(const String& k) const {
        return m_attribute.containsKey(k);
    }

    /** Note that the result is always copied, making this inefficient
        for return values that are not VALUEs. */
    XML get(const String& k, const XML& defaultVal) const {
        const XML* x = m_attribute.getPointer(k);
        if (x) {
            return *x;
        } else {
            return defaultVal;
        }
    }

    Type type() const {
        return m_type;
    }

    /** The name, if this is a TAG. */
    const String name() const {
        return m_name;
    }

    /** Returns "" if a TAG. */
    const String& string() const {
        return m_value;
    }

    /** Parse as a number.  Returns nan() if a TAG or unparseable as a number. */
    double number() const;

    /** Returns false if a TAG. */
    bool boolean() const;

    operator String() const {
        return m_value;
    }

    const String& value() const {
        return m_value;
    }

    operator bool() const {
        return boolean();
    }

    operator double() const {
        return number();
    }

    operator float() const {
        return float(number());
    }

    operator int() const {
        return iRound(number());
    }

}; // class XML

} // namespace G3D
