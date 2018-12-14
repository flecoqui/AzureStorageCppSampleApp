//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
// BrowseASCpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"


typedef size_t udiff_t;
typedef ptrdiff_t diff_t;
typedef std::vector<std::wstring> tstringList;


enum MetaOperation
{
	NONE = 0x000,
	BOL = 0x001,
	ANY = 0x002,
	SPLIT = 0x004,
	ONE = 0x008,
	BEGIN = 0x010,
	END = 0x020,
	MORE = 0x040,
	SINGLE = 0x080,
	ANYSP = 0x100,
	EOL = 0x200,
};

enum MetaSymbols
{
	MS_ANY = _T('*'), // {0, ~}
	MS_SPLIT = _T('|'), // str1 or str2 or ...
	MS_ONE = _T('?'), // {0, 1}, ??? - {0, 3}, ...
	MS_BEGIN = _T('^'), // [str... or [str1... |[str2...
	MS_END = _T('$'), // ...str] or ...str1]| ...str2]
	MS_MORE = _T('+'), // {1, ~}
	MS_SINGLE = _T('#'), // {1}
	MS_ANYSP = _T('>'), // as [^/]*  //TODO: >\>/ i.e. '>' + {symbol}
};

struct Mask
{
	MetaOperation curr;
	MetaOperation prev;
	Mask() : curr(BOL), prev(BOL) {};
};
/**
 * to wildcards
 */
struct Item
{
	std::wstring curr;
	udiff_t pos;
	udiff_t left;
	udiff_t delta;
	Mask mask;
	unsigned short int overlay;

	/** enough of this.. */
	std::wstring prev;
	void reset()
	{
		pos = left = delta = overlay = 0;
		mask.curr = mask.prev = BOL;
		curr.clear();
		prev.clear();
	};
	Item() : pos(0), left(0), delta(0), overlay(0) {};
} item;



/**
 * to words
 */
struct Words
{
	udiff_t found;
	udiff_t left;

	void reset()
	{
		found = std::wstring::npos;
		left = 0;
	};
	Words() : left(0) {};
} words;

inline std::wstring _lowercase(std::wstring str) throw()
{
	transform(str.begin(), str.end(), str.begin(), towlower);
	return str;
};

inline bool rewindToNextBlock(std::wstring& filter, std::wstring::const_iterator& it, bool delta = true)
{
	item.left = filter.find(MS_SPLIT, item.left);
	if (item.left == std::wstring::npos) {
		return false; //EOL
	}
	if (delta) {
		it += ++item.left - (item.pos + item.delta) - 1; //or overload =
	}
	else {
		it += ++item.left - item.pos;
	}
	item.pos = item.left;
	words.left = 0;
	item.mask.prev = SPLIT;
	return true;
}
const static TCHAR ANYSP_CMP_DEFAULT = _T('/');

udiff_t interval(const std::wstring& text)
{
	// "#"
	if (item.mask.prev & SINGLE && (words.found - words.left) != 1)
	{
		udiff_t len = item.prev.length();
		diff_t lPos = words.found - len - item.overlay - 1;
		// [pro]ject ... [pro]t[ection] -> [pro]<-#-ection
		if (lPos < 0 || text.substr(lPos, len).compare(item.prev) != 0) {
			return std::wstring::npos;
		}
		return words.found;
	}
	// "?"
	if (item.mask.prev & ONE && (words.found - words.left) > 1)
	{
		udiff_t len = item.prev.length();
		udiff_t lPosMax = words.found - len;
		udiff_t lPos = lPosMax - item.overlay - 1;
		do { // ????? - min<->max:
			if (text.substr(lPos, len).compare(item.prev) == 0) {
				return words.found;
			}
		} while (++lPos < lPosMax);
		return std::wstring::npos;
	}
	// ">"
	if (item.mask.prev & ANYSP)
	{
		std::wstring inside = text.substr(words.left, words.found - words.left);
		if (inside.find(ANYSP_CMP_DEFAULT) != std::wstring::npos) {
			return std::wstring::npos;
		}
		return words.found;
	}
	//....
	return words.found;
}
bool Match(const std::wstring& text, const std::wstring& filter, bool ignoreCase)
{
	std::wstring _text;
	std::wstring _filter;
	if (filter.empty()) {
		return true;
	}
	if (ignoreCase) {
		//TODO: [perfomance] by single char for iterator
		_text = _lowercase(text);
		_filter = _lowercase(filter); //: ~18ms
	}
	else {
		_text = text;
		_filter = filter;
	}
	item.reset();
	words.reset();

	for (std::wstring::const_iterator it = _filter.begin(), itEnd = _filter.end(); it != itEnd; ++it)
	{
		++item.left;
		switch (*it) {
		case MS_ANY: {
			item.mask.curr = ANY;
			break;
		}
		case MS_ANYSP: {
			item.mask.curr = ANYSP;
			break;
		}
		case MS_SPLIT: {
			item.mask.curr = SPLIT;
			break;
		}
		case MS_ONE: {
			item.mask.curr = ONE;
			break;
		}
		case MS_BEGIN: {
			item.mask.curr = BEGIN;
			break;
		}
		case MS_END: {
			item.mask.curr = END;
			break;
		}
		case MS_MORE: {
			item.mask.curr = MORE;
			break;
		}
		case MS_SINGLE: {
			item.mask.curr = SINGLE;
			break;
		}
		default: {
			if (it + 1 == itEnd) {
				item.mask.curr = EOL;
				++item.left;
			}
			else {
				continue;
			}
		}
		}
		/* When previous symbol is a meta-symbol - delta equal 0 */
		if ((item.delta = item.left - 1 - item.pos) == 0)
		{
			if (item.mask.curr & (SPLIT | EOL)) {
				return true;
			}
			if (item.mask.curr & BEGIN && (item.mask.prev & (BOL | SPLIT)) == 0) // is not: BOL^__ or SPLIT^__
			{
				if (rewindToNextBlock(_filter,it)) { continue; } return false;
			}
			else if (item.mask.curr & END) // combination found, e.g.: *$, ??$, etc. TODO: stub - _stubENDCombination()
			{
				if (rewindToNextBlock(_filter, it)) { continue; } return false;
			}
			// Sequential combinations of characters SINGLE & ONE
			if ((item.mask.curr & SINGLE && item.mask.prev & SINGLE) ||
				(item.mask.curr & ONE && item.mask.prev & ONE)) {
				++item.overlay;
			}
			else { item.overlay = 0; }

			// disable all combinations for SINGLE. TODO: stub - _stubSINGLECombination()
			if ((item.mask.prev & (BOL | EOL)) == 0 &&
				(
				(item.mask.curr & SINGLE && (item.mask.prev & SINGLE) == 0) ||
					(item.mask.prev & SINGLE && (item.mask.curr & SINGLE) == 0)))
			{
				if (rewindToNextBlock(_filter,it)) { continue; } return false;
			}
			++item.pos;
			// TODO: implement non-sequential MS combinations ~ unsigned short int ..
			if ((item.mask.prev & ANYSP) == 0) {
				item.mask.prev = item.mask.curr;
			}
			continue;
		}
		/* Otherwise work with a part of word ... */
		if (item.mask.curr & BEGIN) { // __^xxx
			if (rewindToNextBlock(_filter,it)) { continue; } return false;
		}
		// getting of current word
		item.curr = _filter.substr(item.pos, item.delta);
		if (item.mask.curr & END)
		{
			if (item.delta <= _text.length()
				&& _text.substr(_text.length() - item.delta).compare(item.curr) == 0)
			{
				if (it + 1 == itEnd) { return true; /*EOL*/ }
				switch (*(it + 1)) {
				case MS_SPLIT: {
					return true;
				}
				}
			}
			// __$x
			if (rewindToNextBlock(_filter,it)) { continue; } return false;
		}
		if (item.mask.prev & BEGIN)
		{
			if (_text.substr(0, item.delta).compare(item.curr) == 0) {
				if (item.mask.curr & (SPLIT | EOL)) {
					return true;
				}
				words.found = 0;
			}
			else {
				if (item.mask.curr & EOL) {
					return false;
				}
				if (item.mask.curr & SPLIT) {
					continue;
				}
				if (rewindToNextBlock(_filter,it)) { continue; } return false;
			}
		}
		else {
			// find a part
			if (item.mask.prev & MORE) {
				++words.left;
			}
			words.found = _text.find(item.curr, words.left);
		}
		// working with an interval
		if (words.found != std::wstring::npos) {
			words.found = interval(_text);
		}
		item.overlay = 0; //flush sequence
		/* SPLIT control */
		if (words.found == std::wstring::npos) {
			if (item.mask.curr & EOL) { //TODO: [optimize]: ...or last split-block
				return false;
			}
			item.pos = item.left;
			if (item.mask.curr & SPLIT) {
				continue; //to next block
			}
			if (rewindToNextBlock(_filter,it, false)) { continue; } return false;
		}
		/* Success: */
		if (item.mask.curr & (SPLIT | EOL)) {
			return true;
		}
		item.pos = item.left;
		words.left = words.found + item.delta;
		item.mask.prev = item.mask.curr;
		item.prev = item.curr;
	}
	/* after ending iteration: */
	if (item.mask.prev & MORE && words.left > 0) { // e.g: {word}EOL + {1,~}
		return false;
	}
	if (item.mask.prev & SINGLE) {
		if (words.left == _text.length()) { // e.g: {word}EOL + {1}
			return false;
		}
	}
	if (item.mask.prev & ANYSP) { // {word}>***??* etc.
		if (_text.substr(words.left).find(ANYSP_CMP_DEFAULT) != std::wstring::npos) {
			return false;
		}
	}
	return true;
}
