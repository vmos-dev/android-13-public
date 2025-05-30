/* GENERATED SOURCE. DO NOT MODIFY. */
// ? 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *******************************************************************************
 * Copyright (C) 2012-2015, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */

package android.icu.text;


/**
 * Display context settings.
 * Note, the specific numeric values are internal and may change.
 */

@SuppressWarnings({"unchecked", "deprecation", "all"})
public enum DisplayContext {
/**
 * A possible setting for DIALECT_HANDLING:
 * use standard names when generating a locale name,
 * e.g. en_GB displays as 'English (United Kingdom)'.
 */

STANDARD_NAMES,
/**
 * A possible setting for DIALECT_HANDLING:
 * use dialect names, when generating a locale name,
 * e.g. en_GB displays as 'British English'.
 */

DIALECT_NAMES,
/**
 * A possible setting for CAPITALIZATION:
 * The capitalization context to be used is unknown (this is the default value).
 */

CAPITALIZATION_NONE,
/**
 * A possible setting for CAPITALIZATION:
 * The capitalization context if a date, date symbol or display name is to be
 * formatted with capitalization appropriate for the middle of a sentence.
 */

CAPITALIZATION_FOR_MIDDLE_OF_SENTENCE,
/**
 * A possible setting for CAPITALIZATION:
 * The capitalization context if a date, date symbol or display name is to be
 * formatted with capitalization appropriate for the beginning of a sentence.
 */

CAPITALIZATION_FOR_BEGINNING_OF_SENTENCE,
/**
 * A possible setting for CAPITALIZATION:
 * The capitalization context if a date, date symbol or display name is to be
 * formatted with capitalization appropriate for a user-interface list or menu item.
 */

CAPITALIZATION_FOR_UI_LIST_OR_MENU,
/**
 * A possible setting for CAPITALIZATION:
 * The capitalization context if a date, date symbol or display name is to be
 * formatted with capitalization appropriate for stand-alone usage such as an
 * isolated name on a calendar page.
 */

CAPITALIZATION_FOR_STANDALONE,
/**
 * A possible setting for DISPLAY_LENGTH:
 * use full names when generating a locale name,
 * e.g. "United States" for US.
 */

LENGTH_FULL,
/**
 * A possible setting for DISPLAY_LENGTH:
 * use short names when generating a locale name,
 * e.g. "U.S." for US.
 */

LENGTH_SHORT,
/**
 * A possible setting for SUBSTITUTE_HANDLING:
 * Returns a fallback value (e.g., the input code) when no data is available.
 * This is the default behavior.
 */

SUBSTITUTE,
/**
 * A possible setting for SUBSTITUTE_HANDLING:
 * Returns a null value when no data is available.
 */

NO_SUBSTITUTE;

/**
 * Get the Type part of the enum item
 * (e.g. CAPITALIZATION)
 */

public android.icu.text.DisplayContext.Type type() { throw new RuntimeException("Stub!"); }

/**
 * Get the value part of the enum item
 * (e.g. CAPITALIZATION_FOR_STANDALONE)
 */

public int value() { throw new RuntimeException("Stub!"); }
/**
 * Type values for DisplayContext
 */

@SuppressWarnings({"unchecked", "deprecation", "all"})
public enum Type {
/**
 * DIALECT_HANDLING can be set to STANDARD_NAMES or DIALECT_NAMES.
 */

DIALECT_HANDLING,
/**
 * CAPITALIZATION can be set to one of CAPITALIZATION_NONE through
 * CAPITALIZATION_FOR_STANDALONE.
 */

CAPITALIZATION,
/**
 * DISPLAY_LENGTH can be set to LENGTH_FULL or LENGTH_SHORT.
 */

DISPLAY_LENGTH,
/**
 * SUBSTITUTE_HANDLING can be set to SUBSTITUTE or NO_SUBSTITUTE.
 */

SUBSTITUTE_HANDLING;
}

}

