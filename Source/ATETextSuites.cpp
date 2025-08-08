//========================================================================================
//  
//  ATETextSuites.cpp
//
//  Copyright 2024 Adobe Systems Incorporated. All rights reserved.
//  
//  NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance 
//  with the terms of the Adobe license agreement accompanying it.  If you have received
//  this file from a source other than Adobe, then your use, modification, or 
//  distribution of it requires the prior written permission of Adobe.
//  
//========================================================================================

#include "IllustratorSDK.h"
#include "ATESuites.h"

// Define ALL the ATE suite pointers that IText.cpp references
namespace ATE
{
	// Text object suites
	ApplicationPaintSuite* sApplicationPaint = nullptr;
	CompFontSuite* sCompFont = nullptr;
	CompFontClassSuite* sCompFontClass = nullptr;
	CompFontClassSetSuite* sCompFontClassSet = nullptr;
	CompFontComponentSuite* sCompFontComponent = nullptr;
	CompFontSetSuite* sCompFontSet = nullptr;
	GlyphRunSuite* sGlyphRun = nullptr;
	GlyphRunsIteratorSuite* sGlyphRunsIterator = nullptr;
	ListStyleSuite* sListStyle = nullptr;
	ListStyleSetSuite* sListStyleSet = nullptr;
	MojiKumiSuite* sMojiKumi = nullptr;
	MojiKumiSetSuite* sMojiKumiSet = nullptr;
	TextFrameSuite* sTextFrame = nullptr;
	TextFramesIteratorSuite* sTextFramesIterator = nullptr;
	TextLineSuite* sTextLine = nullptr;
	TextLinesIteratorSuite* sTextLinesIterator = nullptr;
	TextResourcesSuite* sTextResources = nullptr;
	ApplicationTextResourcesSuite* sApplicationTextResources = nullptr;
	DocumentTextResourcesSuite* sDocumentTextResources = nullptr;
	VersionInfoSuite* sVersionInfo = nullptr;
	
	// Array suites
	ArrayApplicationPaintRefSuite* sArrayApplicationPaintRef = nullptr;
	ArrayRealSuite* sArrayReal = nullptr;
	ArrayBoolSuite* sArrayBool = nullptr;
	ArrayIntegerSuite* sArrayInteger = nullptr;
	ArrayLineCapTypeSuite* sArrayLineCapType = nullptr;
	ArrayFigureStyleSuite* sArrayFigureStyle = nullptr;
	ArrayLineJoinTypeSuite* sArrayLineJoinType = nullptr;
	ArrayWariChuJustificationSuite* sArrayWariChuJustification = nullptr;
	ArrayStyleRunAlignmentSuite* sArrayStyleRunAlignment = nullptr;
	ArrayAutoKernTypeSuite* sArrayAutoKernType = nullptr;
	ArrayBaselineDirectionSuite* sArrayBaselineDirection = nullptr;
	ArrayLanguageSuite* sArrayLanguage = nullptr;
	ArrayFontCapsOptionSuite* sArrayFontCapsOption = nullptr;
	ArrayFontBaselineOptionSuite* sArrayFontBaselineOption = nullptr;
	ArrayFontOpenTypePositionOptionSuite* sArrayFontOpenTypePositionOption = nullptr;
	ArrayUnderlinePositionSuite* sArrayUnderlinePosition = nullptr;
	ArrayStrikethroughPositionSuite* sArrayStrikethroughPosition = nullptr;
	ArrayParagraphJustificationSuite* sArrayParagraphJustification = nullptr;
	ArrayArrayRealSuite* sArrayArrayReal = nullptr;
	ArrayBurasagariTypeSuite* sArrayBurasagariType = nullptr;
	ArrayPreferredKinsokuOrderSuite* sArrayPreferredKinsokuOrder = nullptr;
	ArrayKinsokuRefSuite* sArrayKinsokuRef = nullptr;
	ArrayListStyleRefSuite* sArrayListStyleRef = nullptr;
	ArrayListStyleSetRefSuite* sArrayListStyleSetRef = nullptr;
	ArrayMojiKumiRefSuite* sArrayMojiKumiRef = nullptr;
	ArrayMojiKumiSetRefSuite* sArrayMojiKumiSetRef = nullptr;
	ArrayTabStopsRefSuite* sArrayTabStopsRef = nullptr;
	ArrayLeadingTypeSuite* sArrayLeadingType = nullptr;
	ArrayFontRefSuite* sArrayFontRef = nullptr;
	ArrayGlyphIDSuite* sArrayGlyphID = nullptr;
	ArrayRealPointSuite* sArrayRealPoint = nullptr;
	ArrayRealMatrixSuite* sArrayRealMatrix = nullptr;
	ArrayLineSuite* sArrayLine = nullptr;
	ArrayComposerEngineSuite* sArrayComposerEngine = nullptr;
	
	// Character and style suites
	CharFeaturesSuite* sCharFeatures = nullptr;
	CharInspectorSuite* sCharInspector = nullptr;
	CharStyleSuite* sCharStyle = nullptr;
	CharStylesSuite* sCharStyles = nullptr;
	CharStylesIteratorSuite* sCharStylesIterator = nullptr;
	FindSuite* sFind = nullptr;
	FontSuite* sFont = nullptr;
	GlyphSuite* sGlyph = nullptr;
	GlyphsSuite* sGlyphs = nullptr;
	GlyphsIteratorSuite* sGlyphsIterator = nullptr;
	KinsokuSuite* sKinsoku = nullptr;
	KinsokuSetSuite* sKinsokuSet = nullptr;
	ParaFeaturesSuite* sParaFeatures = nullptr;
	ParagraphSuite* sParagraph = nullptr;
	ParagraphsIteratorSuite* sParagraphsIterator = nullptr;
	ParaInspectorSuite* sParaInspector = nullptr;
	ParaStyleSuite* sParaStyle = nullptr;
	ParaStylesSuite* sParaStyles = nullptr;
	ParaStylesIteratorSuite* sParaStylesIterator = nullptr;
	SpellSuite* sSpell = nullptr;
	StoriesSuite* sStories = nullptr;
	StorySuite* sStory = nullptr;
	TabStopSuite* sTabStop = nullptr;
	TabStopsSuite* sTabStops = nullptr;
	TabStopsIteratorSuite* sTabStopsIterator = nullptr;
	
	// The main text range suites we use
	TextRangeSuite* sTextRange = nullptr;
	TextRangesSuite* sTextRanges = nullptr;
	TextRangesIteratorSuite* sTextRangesIterator = nullptr;
	TextRunsIteratorSuite* sTextRunsIterator = nullptr;
	WordsIteratorSuite* sWordsIterator = nullptr;
}

// End ATETextSuites.cpp