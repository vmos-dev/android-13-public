/* GENERATED SOURCE. DO NOT MODIFY. */
// ? 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *******************************************************************************
 * Copyright (C) 1996-2016, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */

package android.icu.text;

import java.text.CharacterIterator;

/**
 * Abstract class that defines an API for iteration on text objects.This is an interface for forward and backward
 * iteration and random access into a text object. Forward iteration is done with post-increment and backward iteration
 * is done with pre-decrement semantics, while the <code>java.text.CharacterIterator</code> interface methods provided
 * forward iteration with "pre-increment" and backward iteration with pre-decrement semantics. This API is more
 * efficient for forward iteration over code points. The other major difference is that this API can do both code unit
 * and code point iteration, <code>java.text.CharacterIterator</code> can only iterate over code units and is limited to
 * BMP (0 - 0xFFFF)
 *
 * @author Ram
 */

@SuppressWarnings({"unchecked", "deprecation", "all"})
public abstract class UCharacterIterator implements java.lang.Cloneable {

/**
 * Protected default constructor for the subclasses
 */

protected UCharacterIterator() { throw new RuntimeException("Stub!"); }

/**
 * Returns a <code>UCharacterIterator</code> object given a <code>Replaceable</code> object.
 *
 * @param source
 *            a valid source as a <code>Replaceable</code> object
 * @return UCharacterIterator object
 * @exception java.lang.IllegalArgumentException
 *                if the argument is null
 */

public static final android.icu.text.UCharacterIterator getInstance(android.icu.text.Replaceable source) { throw new RuntimeException("Stub!"); }

/**
 * Returns a <code>UCharacterIterator</code> object given a source string.
 *
 * @param source
 *            a string
 * @return UCharacterIterator object
 * @exception java.lang.IllegalArgumentException
 *                if the argument is null
 */

public static final android.icu.text.UCharacterIterator getInstance(java.lang.String source) { throw new RuntimeException("Stub!"); }

/**
 * Returns a <code>UCharacterIterator</code> object given a source character array.
 *
 * @param source
 *            an array of UTF-16 code units
 * @return UCharacterIterator object
 * @exception java.lang.IllegalArgumentException
 *                if the argument is null
 */

public static final android.icu.text.UCharacterIterator getInstance(char[] source) { throw new RuntimeException("Stub!"); }

/**
 * Returns a <code>UCharacterIterator</code> object given a source character array.
 *
 * @param source
 *            an array of UTF-16 code units
 * @return UCharacterIterator object
 * @exception java.lang.IllegalArgumentException
 *                if the argument is null
 */

public static final android.icu.text.UCharacterIterator getInstance(char[] source, int start, int limit) { throw new RuntimeException("Stub!"); }

/**
 * Returns a <code>UCharacterIterator</code> object given a source StringBuffer.
 *
 * @param source
 *            an string buffer of UTF-16 code units
 * @return UCharacterIterator object
 * @exception java.lang.IllegalArgumentException
 *                if the argument is null
 */

public static final android.icu.text.UCharacterIterator getInstance(java.lang.StringBuffer source) { throw new RuntimeException("Stub!"); }

/**
 * Returns a <code>UCharacterIterator</code> object given a CharacterIterator.
 *
 * @param source
 *            a valid CharacterIterator object.
 * @return UCharacterIterator object
 * @exception java.lang.IllegalArgumentException
 *                if the argument is null
 */

public static final android.icu.text.UCharacterIterator getInstance(java.text.CharacterIterator source) { throw new RuntimeException("Stub!"); }

/**
 * Returns a <code>java.text.CharacterIterator</code> object for the underlying text of this iterator. The returned
 * iterator is independent of this iterator.
 *
 * @return java.text.CharacterIterator object
 */

public java.text.CharacterIterator getCharacterIterator() { throw new RuntimeException("Stub!"); }

/**
 * Returns the code unit at the current index. If index is out of range, returns DONE. Index is not changed.
 *
 * @return current code unit
 */

public abstract int current();

/**
 * Returns the codepoint at the current index. If the current index is invalid, DONE is returned. If the current
 * index points to a lead surrogate, and there is a following trail surrogate, then the code point is returned.
 * Otherwise, the code unit at index is returned. Index is not changed.
 *
 * @return current codepoint
 */

public int currentCodePoint() { throw new RuntimeException("Stub!"); }

/**
 * Returns the length of the text
 *
 * @return length of the text
 */

public abstract int getLength();

/**
 * Gets the current index in text.
 *
 * @return current index in text.
 */

public abstract int getIndex();

/**
 * Returns the UTF16 code unit at index, and increments to the next code unit (post-increment semantics). If index
 * is out of range, DONE is returned, and the iterator is reset to the limit of the text.
 *
 * @return the next UTF16 code unit, or DONE if the index is at the limit of the text.
 */

public abstract int next();

/**
 * Returns the code point at index, and increments to the next code point (post-increment semantics). If index does
 * not point to a valid surrogate pair, the behavior is the same as <code>next()</code>. Otherwise the iterator is
 * incremented past the surrogate pair, and the code point represented by the pair is returned.
 *
 * @return the next codepoint in text, or DONE if the index is at the limit of the text.
 */

public int nextCodePoint() { throw new RuntimeException("Stub!"); }

/**
 * Decrement to the position of the previous code unit in the text, and return it (pre-decrement semantics). If the
 * resulting index is less than 0, the index is reset to 0 and DONE is returned.
 *
 * @return the previous code unit in the text, or DONE if the new index is before the start of the text.
 */

public abstract int previous();

/**
 * Retreat to the start of the previous code point in the text, and return it (pre-decrement semantics). If the
 * index is not preceeded by a valid surrogate pair, the behavior is the same as <code>previous()</code>. Otherwise
 * the iterator is decremented to the start of the surrogate pair, and the code point represented by the pair is
 * returned.
 *
 * @return the previous code point in the text, or DONE if the new index is before the start of the text.
 */

public int previousCodePoint() { throw new RuntimeException("Stub!"); }

/**
 * Sets the index to the specified index in the text.
 *
 * @param index
 *            the index within the text.
 * @exception java.lang.IndexOutOfBoundsException
 *                is thrown if an invalid index is supplied
 */

public abstract void setIndex(int index);

/**
 * Sets the current index to the limit.
 */

public void setToLimit() { throw new RuntimeException("Stub!"); }

/**
 * Sets the current index to the start.
 */

public void setToStart() { throw new RuntimeException("Stub!"); }

/**
 * Fills the buffer with the underlying text storage of the iterator If the buffer capacity is not enough a
 * exception is thrown. The capacity of the fill in buffer should at least be equal to length of text in the
 * iterator obtained by calling <code>getLength()</code>). <b>Usage:</b>
 *
 * <pre>
 *         UChacterIterator iter = new UCharacterIterator.getInstance(text);
 *         char[] buf = new char[iter.getLength()];
 *         iter.getText(buf);
 *
 *         OR
 *         char[] buf= new char[1];
 *         int len = 0;
 *         for(;;){
 *             try{
 *                 len = iter.getText(buf);
 *                 break;
 *             }catch(IndexOutOfBoundsException e){
 *                 buf = new char[iter.getLength()];
 *             }
 *         }
 * </pre>
 *
 * @param fillIn
 *            an array of chars to fill with the underlying UTF-16 code units.
 * @param offset
 *            the position within the array to start putting the data.
 * @return the number of code units added to fillIn, as a convenience
 * @exception java.lang.IndexOutOfBoundsException
 *                exception if there is not enough room after offset in the array, or if offset &lt; 0.
 */

public abstract int getText(char[] fillIn, int offset);

/**
 * Convenience override for <code>getText(char[], int)</code> that provides an offset of 0.
 *
 * @param fillIn
 *            an array of chars to fill with the underlying UTF-16 code units.
 * @return the number of code units added to fillIn, as a convenience
 * @exception java.lang.IndexOutOfBoundsException
 *                exception if there is not enough room in the array.
 */

public final int getText(char[] fillIn) { throw new RuntimeException("Stub!"); }

/**
 * Convenience method for returning the underlying text storage as as string
 *
 * @return the underlying text storage in the iterator as a string
 */

public java.lang.String getText() { throw new RuntimeException("Stub!"); }

/**
 * Moves the current position by the number of code units specified, either forward or backward depending on the
 * sign of delta (positive or negative respectively). If the resulting index would be less than zero, the index is
 * set to zero, and if the resulting index would be greater than limit, the index is set to limit.
 *
 * @param delta
 *            the number of code units to move the current index.
 * @return the new index.
 * @exception java.lang.IndexOutOfBoundsException
 *                is thrown if an invalid index is supplied
 *
 */

public int moveIndex(int delta) { throw new RuntimeException("Stub!"); }

/**
 * Moves the current position by the number of code points specified, either forward or backward depending on the
 * sign of delta (positive or negative respectively). If the current index is at a trail surrogate then the first
 * adjustment is by code unit, and the remaining adjustments are by code points. If the resulting index would be
 * less than zero, the index is set to zero, and if the resulting index would be greater than limit, the index is
 * set to limit.
 *
 * @param delta
 *            the number of code units to move the current index.
 * @return the new index
 * @exception java.lang.IndexOutOfBoundsException
 *                is thrown if an invalid delta is supplied
 */

public int moveCodePointIndex(int delta) { throw new RuntimeException("Stub!"); }

/**
 * Creates a copy of this iterator, independent from other iterators. If it is not possible to clone the iterator,
 * returns null.
 *
 * @return copy of this iterator
 */

public java.lang.Object clone() throws java.lang.CloneNotSupportedException { throw new RuntimeException("Stub!"); }

/**
 * Indicator that we have reached the ends of the UTF16 text.
 */

public static final int DONE = -1; // 0xffffffff
}

