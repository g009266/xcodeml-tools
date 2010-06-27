/*
 * The Relaxer artifact
 * Copyright (c) 2000-2003, ASAMI Tomoharu, All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer. 
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
package xcodeml.f.binding.gen;

import xcodeml.binding.*;

import java.io.*;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.Writer;
import java.net.URL;
import javax.xml.parsers.*;
import org.w3c.dom.*;
import org.xml.sax.*;

/**
 * <b>XbfNamedValueList</b> is generated from XcodeML_F.rng by Relaxer.
 * This class is derived from:
 * 
 * <!-- for programmer
 * <element java:extends="xcodeml.f.XmfObj" name="namedValueList">
 *   <zeroOrMore>
 *     <ref name="namedValue"/>
 *   </zeroOrMore>
 * </element>
 * -->
 * <!-- for javadoc -->
 * <pre> &lt;element java:extends="xcodeml.f.XmfObj" name="namedValueList"&gt;
 *   &lt;zeroOrMore&gt;
 *     &lt;ref name="namedValue"/&gt;
 *   &lt;/zeroOrMore&gt;
 * &lt;/element&gt;
 * </pre>
 *
 * @version XcodeML_F.rng (Fri Dec 04 19:18:15 JST 2009)
 * @author  Relaxer 1.0 (http://www.relaxer.org)
 */
public class XbfNamedValueList extends xcodeml.f.XmfObj implements java.io.Serializable, Cloneable, IRVisitable, IRNode {
    // List<XbfNamedValue>
    private java.util.List namedValue_ = new java.util.ArrayList();
    private IRNode parentRNode_;

    /**
     * Creates a <code>XbfNamedValueList</code>.
     *
     */
    public XbfNamedValueList() {
    }

    /**
     * Creates a <code>XbfNamedValueList</code>.
     *
     * @param source
     */
    public XbfNamedValueList(XbfNamedValueList source) {
        setup(source);
    }

    /**
     * Creates a <code>XbfNamedValueList</code> by the Stack <code>stack</code>
     * that contains Elements.
     * This constructor is supposed to be used internally
     * by the Relaxer system.
     *
     * @param stack
     */
    public XbfNamedValueList(RStack stack) {
        setup(stack);
    }

    /**
     * Creates a <code>XbfNamedValueList</code> by the Document <code>doc</code>.
     *
     * @param doc
     */
    public XbfNamedValueList(Document doc) {
        setup(doc.getDocumentElement());
    }

    /**
     * Creates a <code>XbfNamedValueList</code> by the Element <code>element</code>.
     *
     * @param element
     */
    public XbfNamedValueList(Element element) {
        setup(element);
    }

    /**
     * Creates a <code>XbfNamedValueList</code> by the File <code>file</code>.
     *
     * @param file
     * @exception IOException
     * @exception SAXException
     * @exception ParserConfigurationException
     */
    public XbfNamedValueList(File file) throws IOException, SAXException, ParserConfigurationException {
        setup(file);
    }

    /**
     * Creates a <code>XbfNamedValueList</code>
     * by the String representation of URI <code>uri</code>.
     *
     * @param uri
     * @exception IOException
     * @exception SAXException
     * @exception ParserConfigurationException
     */
    public XbfNamedValueList(String uri) throws IOException, SAXException, ParserConfigurationException {
        setup(uri);
    }

    /**
     * Creates a <code>XbfNamedValueList</code> by the URL <code>url</code>.
     *
     * @param url
     * @exception IOException
     * @exception SAXException
     * @exception ParserConfigurationException
     */
    public XbfNamedValueList(URL url) throws IOException, SAXException, ParserConfigurationException {
        setup(url);
    }

    /**
     * Creates a <code>XbfNamedValueList</code> by the InputStream <code>in</code>.
     *
     * @param in
     * @exception IOException
     * @exception SAXException
     * @exception ParserConfigurationException
     */
    public XbfNamedValueList(InputStream in) throws IOException, SAXException, ParserConfigurationException {
        setup(in);
    }

    /**
     * Creates a <code>XbfNamedValueList</code> by the InputSource <code>is</code>.
     *
     * @param is
     * @exception IOException
     * @exception SAXException
     * @exception ParserConfigurationException
     */
    public XbfNamedValueList(InputSource is) throws IOException, SAXException, ParserConfigurationException {
        setup(is);
    }

    /**
     * Creates a <code>XbfNamedValueList</code> by the Reader <code>reader</code>.
     *
     * @param reader
     * @exception IOException
     * @exception SAXException
     * @exception ParserConfigurationException
     */
    public XbfNamedValueList(Reader reader) throws IOException, SAXException, ParserConfigurationException {
        setup(reader);
    }

    /**
     * Initializes the <code>XbfNamedValueList</code> by the XbfNamedValueList <code>source</code>.
     *
     * @param source
     */
    public void setup(XbfNamedValueList source) {
        int size;
        this.namedValue_.clear();
        size = source.namedValue_.size();
        for (int i = 0;i < size;i++) {
            addNamedValue((XbfNamedValue)source.getNamedValue(i).clone());
        }
    }

    /**
     * Initializes the <code>XbfNamedValueList</code> by the Document <code>doc</code>.
     *
     * @param doc
     */
    public void setup(Document doc) {
        setup(doc.getDocumentElement());
    }

    /**
     * Initializes the <code>XbfNamedValueList</code> by the Element <code>element</code>.
     *
     * @param element
     */
    public void setup(Element element) {
        init(element);
    }

    /**
     * Initializes the <code>XbfNamedValueList</code> by the Stack <code>stack</code>
     * that contains Elements.
     * This constructor is supposed to be used internally
     * by the Relaxer system.
     *
     * @param stack
     */
    public void setup(RStack stack) {
        init(stack.popElement());
    }

    /**
     * @param element
     */
    private void init(Element element) {
        IXcodeML_FFactory factory = XcodeML_FFactory.getFactory();
        RStack stack = new RStack(element);
        namedValue_.clear();
        while (true) {
            if (XbfNamedValue.isMatch(stack)) {
                addNamedValue(factory.createXbfNamedValue(stack));
            } else {
                break;
            }
        }
    }

    /**
     * @return Object
     */
    public Object clone() {
        IXcodeML_FFactory factory = XcodeML_FFactory.getFactory();
        return (factory.createXbfNamedValueList(this));
    }

    /**
     * Creates a DOM representation of the object.
     * Result is appended to the Node <code>parent</code>.
     *
     * @param parent
     */
    public void makeElement(Node parent) {
        Document doc;
        if (parent instanceof Document) {
            doc = (Document)parent;
        } else {
            doc = parent.getOwnerDocument();
        }
        Element element = doc.createElement("namedValueList");
        int size;
        size = this.namedValue_.size();
        for (int i = 0;i < size;i++) {
            XbfNamedValue value = (XbfNamedValue)this.namedValue_.get(i);
            value.makeElement(element);
        }
        parent.appendChild(element);
    }

    /**
     * Initializes the <code>XbfNamedValueList</code> by the File <code>file</code>.
     *
     * @param file
     * @exception IOException
     * @exception SAXException
     * @exception ParserConfigurationException
     */
    public void setup(File file) throws IOException, SAXException, ParserConfigurationException {
        setup(file.toURL());
    }

    /**
     * Initializes the <code>XbfNamedValueList</code>
     * by the String representation of URI <code>uri</code>.
     *
     * @param uri
     * @exception IOException
     * @exception SAXException
     * @exception ParserConfigurationException
     */
    public void setup(String uri) throws IOException, SAXException, ParserConfigurationException {
        setup(UJAXP.getDocument(uri, UJAXP.FLAG_NONE));
    }

    /**
     * Initializes the <code>XbfNamedValueList</code> by the URL <code>url</code>.
     *
     * @param url
     * @exception IOException
     * @exception SAXException
     * @exception ParserConfigurationException
     */
    public void setup(URL url) throws IOException, SAXException, ParserConfigurationException {
        setup(UJAXP.getDocument(url, UJAXP.FLAG_NONE));
    }

    /**
     * Initializes the <code>XbfNamedValueList</code> by the InputStream <code>in</code>.
     *
     * @param in
     * @exception IOException
     * @exception SAXException
     * @exception ParserConfigurationException
     */
    public void setup(InputStream in) throws IOException, SAXException, ParserConfigurationException {
        setup(UJAXP.getDocument(in, UJAXP.FLAG_NONE));
    }

    /**
     * Initializes the <code>XbfNamedValueList</code> by the InputSource <code>is</code>.
     *
     * @param is
     * @exception IOException
     * @exception SAXException
     * @exception ParserConfigurationException
     */
    public void setup(InputSource is) throws IOException, SAXException, ParserConfigurationException {
        setup(UJAXP.getDocument(is, UJAXP.FLAG_NONE));
    }

    /**
     * Initializes the <code>XbfNamedValueList</code> by the Reader <code>reader</code>.
     *
     * @param reader
     * @exception IOException
     * @exception SAXException
     * @exception ParserConfigurationException
     */
    public void setup(Reader reader) throws IOException, SAXException, ParserConfigurationException {
        setup(UJAXP.getDocument(reader, UJAXP.FLAG_NONE));
    }

    /**
     * Creates a DOM document representation of the object.
     *
     * @exception ParserConfigurationException
     * @return Document
     */
    public Document makeDocument() throws ParserConfigurationException {
        Document doc = UJAXP.makeDocument();
        makeElement(doc);
        return (doc);
    }

    /**
     * Gets the XbfNamedValue property <b>namedValue</b>.
     *
     * @return XbfNamedValue[]
     */
    public final XbfNamedValue[] getNamedValue() {
        XbfNamedValue[] array = new XbfNamedValue[namedValue_.size()];
        return ((XbfNamedValue[])namedValue_.toArray(array));
    }

    /**
     * Sets the XbfNamedValue property <b>namedValue</b>.
     *
     * @param namedValue
     */
    public final void setNamedValue(XbfNamedValue[] namedValue) {
        this.namedValue_.clear();
        for (int i = 0;i < namedValue.length;i++) {
            addNamedValue(namedValue[i]);
        }
        for (int i = 0;i < namedValue.length;i++) {
            namedValue[i].rSetParentRNode(this);
        }
    }

    /**
     * Sets the XbfNamedValue property <b>namedValue</b>.
     *
     * @param namedValue
     */
    public final void setNamedValue(XbfNamedValue namedValue) {
        this.namedValue_.clear();
        addNamedValue(namedValue);
        if (namedValue != null) {
            namedValue.rSetParentRNode(this);
        }
    }

    /**
     * Adds the XbfNamedValue property <b>namedValue</b>.
     *
     * @param namedValue
     */
    public final void addNamedValue(XbfNamedValue namedValue) {
        this.namedValue_.add(namedValue);
        if (namedValue != null) {
            namedValue.rSetParentRNode(this);
        }
    }

    /**
     * Adds the XbfNamedValue property <b>namedValue</b>.
     *
     * @param namedValue
     */
    public final void addNamedValue(XbfNamedValue[] namedValue) {
        for (int i = 0;i < namedValue.length;i++) {
            addNamedValue(namedValue[i]);
        }
        for (int i = 0;i < namedValue.length;i++) {
            namedValue[i].rSetParentRNode(this);
        }
    }

    /**
     * Gets number of the XbfNamedValue property <b>namedValue</b>.
     *
     * @return int
     */
    public final int sizeNamedValue() {
        return (namedValue_.size());
    }

    /**
     * Gets the XbfNamedValue property <b>namedValue</b> by index.
     *
     * @param index
     * @return XbfNamedValue
     */
    public final XbfNamedValue getNamedValue(int index) {
        return ((XbfNamedValue)namedValue_.get(index));
    }

    /**
     * Sets the XbfNamedValue property <b>namedValue</b> by index.
     *
     * @param index
     * @param namedValue
     */
    public final void setNamedValue(int index, XbfNamedValue namedValue) {
        this.namedValue_.set(index, namedValue);
        if (namedValue != null) {
            namedValue.rSetParentRNode(this);
        }
    }

    /**
     * Adds the XbfNamedValue property <b>namedValue</b> by index.
     *
     * @param index
     * @param namedValue
     */
    public final void addNamedValue(int index, XbfNamedValue namedValue) {
        this.namedValue_.add(index, namedValue);
        if (namedValue != null) {
            namedValue.rSetParentRNode(this);
        }
    }

    /**
     * Remove the XbfNamedValue property <b>namedValue</b> by index.
     *
     * @param index
     */
    public final void removeNamedValue(int index) {
        this.namedValue_.remove(index);
    }

    /**
     * Remove the XbfNamedValue property <b>namedValue</b> by object.
     *
     * @param namedValue
     */
    public final void removeNamedValue(XbfNamedValue namedValue) {
        this.namedValue_.remove(namedValue);
    }

    /**
     * Clear the XbfNamedValue property <b>namedValue</b>.
     *
     */
    public final void clearNamedValue() {
        this.namedValue_.clear();
    }

    /**
     * Makes an XML text representation.
     *
     * @return String
     */
    public String makeTextDocument() {
        StringBuffer buffer = new StringBuffer();
        makeTextElement(buffer);
        return (new String(buffer));
    }

    /**
     * Makes an XML text representation.
     *
     * @param buffer
     */
    public void makeTextElement(StringBuffer buffer) {
        int size;
        buffer.append("<namedValueList");
        buffer.append(">");
        size = this.namedValue_.size();
        for (int i = 0;i < size;i++) {
            XbfNamedValue value = (XbfNamedValue)this.namedValue_.get(i);
            value.makeTextElement(buffer);
        }
        buffer.append("</namedValueList>");
    }

    /**
     * Makes an XML text representation.
     *
     * @param buffer
     * @exception IOException
     */
    public void makeTextElement(Writer buffer) throws IOException {
        int size;
        buffer.write("<namedValueList");
        buffer.write(">");
        size = this.namedValue_.size();
        for (int i = 0;i < size;i++) {
            XbfNamedValue value = (XbfNamedValue)this.namedValue_.get(i);
            value.makeTextElement(buffer);
        }
        buffer.write("</namedValueList>");
    }

    /**
     * Makes an XML text representation.
     *
     * @param buffer
     */
    public void makeTextElement(PrintWriter buffer) {
        int size;
        buffer.print("<namedValueList");
        buffer.print(">");
        size = this.namedValue_.size();
        for (int i = 0;i < size;i++) {
            XbfNamedValue value = (XbfNamedValue)this.namedValue_.get(i);
            value.makeTextElement(buffer);
        }
        buffer.print("</namedValueList>");
    }

    /**
     * Makes an XML text representation.
     *
     * @param buffer
     */
    public void makeTextAttribute(StringBuffer buffer) {
    }

    /**
     * Makes an XML text representation.
     *
     * @param buffer
     * @exception IOException
     */
    public void makeTextAttribute(Writer buffer) throws IOException {
    }

    /**
     * Makes an XML text representation.
     *
     * @param buffer
     */
    public void makeTextAttribute(PrintWriter buffer) {
    }

    /**
     * Returns a String representation of this object.
     * While this method informs as XML format representaion, 
     *  it's purpose is just information, not making 
     * a rigid XML documentation.
     *
     * @return String
     */
    public String toString() {
        try {
            return (makeTextDocument());
        } catch (Exception e) {
            return (super.toString());
        }
    }

    /**
     * Accepts the Visitor for enter behavior.
     *
     * @param visitor
     * @return boolean
     */
    public boolean enter(IRVisitor visitor) {
        return (visitor.enter(this));
    }

    /**
     * Accepts the Visitor for leave behavior.
     *
     * @param visitor
     */
    public void leave(IRVisitor visitor) {
        visitor.leave(this);
    }

    /**
     * Gets the IRNode property <b>parentRNode</b>.
     *
     * @return IRNode
     */
    public final IRNode rGetParentRNode() {
        return (parentRNode_);
    }

    /**
     * Sets the IRNode property <b>parentRNode</b>.
     *
     * @param parentRNode
     */
    public final void rSetParentRNode(IRNode parentRNode) {
        this.parentRNode_ = parentRNode;
    }

    /**
     * Gets child RNodes.
     *
     * @return IRNode[]
     */
    public IRNode[] rGetRNodes() {
        java.util.List classNodes = new java.util.ArrayList();
        classNodes.addAll(namedValue_);
        IRNode[] nodes = new IRNode[classNodes.size()];
        return ((IRNode[])classNodes.toArray(nodes));
    }

    /**
     * Tests if a Element <code>element</code> is valid
     * for the <code>XbfNamedValueList</code>.
     *
     * @param element
     * @return boolean
     */
    public static boolean isMatch(Element element) {
        if (!URelaxer.isTargetElement(element, "namedValueList")) {
            return (false);
        }
        RStack target = new RStack(element);
        boolean $match$ = false;
        Element child;
        while (true) {
            if (!XbfNamedValue.isMatchHungry(target)) {
                break;
            }
            $match$ = true;
        }
        if (!target.isEmptyElement()) {
            return (false);
        }
        return (true);
    }

    /**
     * Tests if elements contained in a Stack <code>stack</code>
     * is valid for the <code>XbfNamedValueList</code>.
     * This mehtod is supposed to be used internally
     * by the Relaxer system.
     *
     * @param stack
     * @return boolean
     */
    public static boolean isMatch(RStack stack) {
        Element element = stack.peekElement();
        if (element == null) {
            return (false);
        }
        return (isMatch(element));
    }

    /**
     * Tests if elements contained in a Stack <code>stack</code>
     * is valid for the <code>XbfNamedValueList</code>.
     * This method consumes the stack contents during matching operation.
     * This mehtod is supposed to be used internally
     * by the Relaxer system.
     *
     * @param stack
     * @return boolean
     */
    public static boolean isMatchHungry(RStack stack) {
        Element element = stack.peekElement();
        if (element == null) {
            return (false);
        }
        if (isMatch(element)) {
            stack.popElement();
            return (true);
        } else {
            return (false);
        }
    }
}
