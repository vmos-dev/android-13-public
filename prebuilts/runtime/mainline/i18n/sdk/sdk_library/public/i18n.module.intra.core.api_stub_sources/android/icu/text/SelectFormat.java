/* GENERATED SOURCE. DO NOT MODIFY. */
// ? 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *******************************************************************************
 * Copyright (C) 2004-2016, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 * Copyright (C) 2009 , Yahoo! Inc.                                            *
 *******************************************************************************
 */

package android.icu.text;


/**
 * <p><code>SelectFormat</code> supports the creation of  internationalized
 * messages by selecting phrases based on keywords. The pattern  specifies
 * how to map keywords to phrases and provides a default phrase. The
 * object provided to the format method is a string that's matched
 * against the keywords. If there is a match, the corresponding phrase
 * is selected; otherwise, the default phrase is used.
 *
 * <h3>Using <code>SelectFormat</code> for Gender Agreement</h3>
 *
 * <p>Note: Typically, select formatting is done via <code>MessageFormat</code>
 * with a <code>select</code> argument type,
 * rather than using a stand-alone <code>SelectFormat</code>.
 *
 * <p>The main use case for the select format is gender based  inflection.
 * When names or nouns are inserted into sentences, their gender can  affect pronouns,
 * verb forms, articles, and adjectives. Special care needs to be
 * taken for the case where the gender cannot be determined.
 * The impact varies between languages:
 *
 * <ul>
 * <li>English has three genders, and unknown gender is handled as a  special
 * case. Names use the gender of the named person (if known), nouns  referring
 * to people use natural gender, and inanimate objects are usually  neutral.
 * The gender only affects pronouns: "he", "she", "it", "they".
 *
 * <li>German differs from English in that the gender of nouns is  rather
 * arbitrary, even for nouns referring to people ("M&#xE4;dchen", girl, is  neutral).
 * The gender affects pronouns ("er", "sie", "es"), articles ("der",  "die",
 * "das"), and adjective forms ("guter Mann", "gute Frau", "gutes  M&#xE4;dchen").
 *
 * <li>French has only two genders; as in German the gender of nouns
 * is rather arbitrary - for sun and moon, the genders
 * are the opposite of those in German. The gender affects
 * pronouns ("il", "elle"), articles ("le", "la"),
 * adjective forms ("bon", "bonne"), and sometimes
 * verb forms ("all&#xE9;", "all&#xE9;e").
 *
 * <li>Polish distinguishes five genders (or noun classes),
 * human masculine, animate non-human masculine, inanimate masculine,
 * feminine, and neuter.
 * </ul>
 *
 * <p>Some other languages have noun classes that are not related to  gender,
 * but similar in grammatical use.
 * Some African languages have around 20 noun classes.
 *
 * <p><b>Note:</b>For the gender of a <i>person</i> in a given sentence,
 * we usually need to distinguish only between female, male and other/unknown.
 *
 * <p>To enable localizers to create sentence patterns that take their
 * language's gender dependencies into consideration, software has to  provide
 * information about the gender associated with a noun or name to
 * <code>MessageFormat</code>.
 * Two main cases can be distinguished:
 *
 * <ul>
 * <li>For people, natural gender information should be maintained  for each person.
 * Keywords like "male", "female", "mixed" (for groups of people)
 * and "unknown" could be used.
 *
 * <li>For nouns, grammatical gender information should be maintained  for
 * each noun and per language, e.g., in resource bundles.
 * The keywords "masculine", "feminine", and "neuter" are commonly  used,
 * but some languages may require other keywords.
 * </ul>
 *
 * <p>The resulting keyword is provided to <code>MessageFormat</code>  as a
 * parameter separate from the name or noun it's associated with. For  example,
 * to generate a message such as "Jean went to Paris", three separate  arguments
 * would be provided: The name of the person as argument 0, the  gender of
 * the person as argument 1, and the name of the city as argument 2.
 * The sentence pattern for English, where the gender of the person has
 * no impact on this simple sentence, would not refer to argument 1  at all:
 *
 * <pre>{0} went to {2}.</pre>
 *
 * <p><b>Note:</b> The entire sentence should be included (and partially repeated)
 * inside each phrase. Otherwise translators would have to be trained on how to
 * move bits of the sentence in and out of the select argument of a message.
 * (The examples below do not follow this recommendation!)
 *
 * <p>The sentence pattern for French, where the gender of the person affects
 * the form of the participle, uses a select format based on argument 1:
 *
 * <pre>{0} est {1, select, female {all&#xE9;e} other {all&#xE9;}} &#xE0; {2}.</pre>
 *
 * <p>Patterns can be nested, so that it's possible to handle  interactions of
 * number and gender where necessary. For example, if the above  sentence should
 * allow for the names of several people to be inserted, the  following sentence
 * pattern can be used (with argument 0 the list of people's names,
 * argument 1 the number of people, argument 2 their combined gender, and
 * argument 3 the city name):
 *
 * <pre>{0} {1, plural,
 * one {est {2, select, female {all&#xE9;e} other  {all&#xE9;}}}
 * other {sont {2, select, female {all&#xE9;es} other {all&#xE9;s}}}
 * }&#xE0; {3}.</pre>
 *
 * <h4>Patterns and Their Interpretation</h4>
 *
 * <p>The <code>SelectFormat</code> pattern string defines the phrase  output
 * for each user-defined keyword.
 * The pattern is a sequence of (keyword, message) pairs.
 * A keyword is a "pattern identifier": [^[[:Pattern_Syntax:][:Pattern_White_Space:]]]+
 *
 * <p>Each message is a MessageFormat pattern string enclosed in {curly braces}.
 *
 * <p>You always have to define a phrase for the default keyword
 * <code>other</code>; this phrase is returned when the keyword
 * provided to
 * the <code>format</code> method matches no other keyword.
 * If a pattern does not provide a phrase for <code>other</code>, the  method
 * it's provided to returns the error  <code>U_DEFAULT_KEYWORD_MISSING</code>.
 * <br>
 * Pattern_White_Space between keywords and messages is ignored.
 * Pattern_White_Space within a message is preserved and output.
 *
 * <pre>Example:
 * MessageFormat msgFmt = new MessageFormat("{0} est " +
 *     "{1, select, female {all&#xE9;e} other {all&#xE9;}} &#xE0; Paris.",
 *     new ULocale("fr"));
 * Object args[] = {"Kirti","female"};
 * System.out.println(msgFmt.format(args));
 * </pre>
 * <p>
 * Produces the output:<br>
 * <code>Kirti est all&#xE9;e &#xE0; Paris.</code>
 */

@SuppressWarnings({"unchecked", "deprecation", "all"})
public class SelectFormat extends java.text.Format {

/**
 * Creates a new <code>SelectFormat</code> for a given pattern string.
 * @param  pattern the pattern for this <code>SelectFormat</code>.
 */

public SelectFormat(java.lang.String pattern) { throw new RuntimeException("Stub!"); }

/**
 * Sets the pattern used by this select format.
 * Patterns and their interpretation are specified in the class description.
 *
 * @param pattern the pattern for this select format.
 * @throws java.lang.IllegalArgumentException when the pattern is not a valid select format pattern.
 */

public void applyPattern(java.lang.String pattern) { throw new RuntimeException("Stub!"); }

/**
 * Returns the pattern for this <code>SelectFormat</code>
 *
 * @return the pattern string
 */

public java.lang.String toPattern() { throw new RuntimeException("Stub!"); }

/**
 * Selects the phrase for the given keyword.
 *
 * @param keyword a phrase selection keyword.
 * @return the string containing the formatted select message.
 * @throws java.lang.IllegalArgumentException when the given keyword is not a "pattern identifier"
 */

public final java.lang.String format(java.lang.String keyword) { throw new RuntimeException("Stub!"); }

/**
 * Selects the phrase for the given keyword.
 * and appends the formatted message to the given <code>StringBuffer</code>.
 * @param keyword a phrase selection keyword.
 * @param toAppendTo the selected phrase will be appended to this
 *        <code>StringBuffer</code>.
 * @param pos will be ignored by this method.
 * @throws java.lang.IllegalArgumentException when the given keyword is not a String
 *         or not a "pattern identifier"
 * @return the string buffer passed in as toAppendTo, with formatted text
 *         appended.
 */

public java.lang.StringBuffer format(java.lang.Object keyword, java.lang.StringBuffer toAppendTo, java.text.FieldPosition pos) { throw new RuntimeException("Stub!"); }

/**
 * This method is not supported by <code>SelectFormat</code>.
 * @param source the string to be parsed.
 * @param pos defines the position where parsing is to begin,
 * and upon return, the position where parsing left off.  If the position
 * has not changed upon return, then parsing failed.
 * @return nothing because this method is not supported.
 * @throws java.lang.UnsupportedOperationException thrown always.
 */

public java.lang.Object parseObject(java.lang.String source, java.text.ParsePosition pos) { throw new RuntimeException("Stub!"); }

/**
 * {@inheritDoc}
 */

public boolean equals(java.lang.Object obj) { throw new RuntimeException("Stub!"); }

/**
 * {@inheritDoc}
 */

public int hashCode() { throw new RuntimeException("Stub!"); }

/**
 * {@inheritDoc}
 */

public java.lang.String toString() { throw new RuntimeException("Stub!"); }
}

