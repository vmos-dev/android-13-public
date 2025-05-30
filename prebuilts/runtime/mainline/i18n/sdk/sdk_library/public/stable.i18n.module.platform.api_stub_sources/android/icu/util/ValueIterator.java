/* GENERATED SOURCE. DO NOT MODIFY. */
// ? 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
******************************************************************************
* Copyright (C) 1996-2016, International Business Machines Corporation and   *
* others. All Rights Reserved.                                               *
******************************************************************************
*/


package android.icu.util;


/**
 * <p>Interface for enabling iteration over sets of &lt;int, Object&gt;, where
 * int is the sorted integer index in ascending order, and Object its
 * associated value.
 * <p>The ValueIterator allows iterations over integer indexes in the range
 * of Integer.MIN_VALUE to Integer.MAX_VALUE inclusive. Implementations of
 * ValueIterator should specify their own maximum subrange within the above
 * range that is meaningful to its applications.
 * <p>Most implementations will be created by factory methods, such as the
 * character name iterator in UCharacter.getNameIterator. See example below.
 *
 * Example of use:<br>
 * <pre>
 * ValueIterator iterator = UCharacter.getNameIterator();
 * ValueIterator.Element result = new ValueIterator.Element();
 * iterator.setRange(UCharacter.MIN_VALUE, UCharacter.MAX_VALUE);
 * while (iterator.next(result)) {
 *     System.out.println("Codepoint \\u" +
 *                        Integer.toHexString(result.integer) +
 *                        " has the character name " + (String)result.value);
 * }
 * </pre>
 * @author synwee
 */

@SuppressWarnings({"unchecked", "deprecation", "all"})
public interface ValueIterator {

/**
 * <p>Returns the next result for this iteration and returns
 * true if we are not at the end of the iteration, false otherwise.
 * <p>If this returns a false, the contents of elements will not
 * be updated.
 * @param element for storing the result index and value
 * @return true if we are not at the end of the iteration, false otherwise.
 * @see android.icu.util.ValueIterator.Element
 */

public boolean next(android.icu.util.ValueIterator.Element element);

/**
 * <p>Resets the iterator to start iterating from the integer index
 * Integer.MIN_VALUE or X if a setRange(X, Y) has been called previously.
 */

public void reset();

/**
 * <p>Restricts the range of integers to iterate and resets the iteration
 * to begin at the index argument start.
 * <p>If setRange(start, end) is not performed before next(element) is
 * called, the iteration will start from the integer index
 * Integer.MIN_VALUE and end at Integer.MAX_VALUE.
 * <p>
 * If this range is set outside the meaningful range specified by the
 * implementation, next(element) will always return false.
 *
 * @param start first integer in the range to iterate
 * @param limit one more than the last integer in the range
 * @exception java.lang.IllegalArgumentException thrown when attempting to set an
 *            illegal range. E.g limit &lt;= start
 */

public void setRange(int start, int limit);
/**
 * <p>The return result container of each iteration. Stores the next
 * integer index and its associated value Object.
 */

@SuppressWarnings({"unchecked", "deprecation", "all"})
public static final class Element {

/**
 * Empty default constructor to make javadoc happy
 */

public Element() { throw new RuntimeException("Stub!"); }

/**
 * Integer index of the current iteration
 */

public int integer;

/**
 * Gets the Object value associated with the integer index.
 */

public java.lang.Object value;
}

}

