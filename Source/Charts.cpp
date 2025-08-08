//========================================================================================
//  
//  $File$
//
//  $Revision$
//
//  Copyright 2008 Adobe Systems Incorporated. All rights reserved.
//  
//  NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance 
//  with the terms of the Adobe license agreement accompanying it.  If you have received
//  this file from a source other than Adobe, then your use, modification, or 
//  distribution of it requires the prior written permission of Adobe.
//  
//========================================================================================

#include "IllustratorSDK.h"
#include "Charts.h"

/** A light yellow color that we will use as the background for our annotations
*/
const AIRGBColor BKGND_COLOR = {65000, 65000, 40000};

/** Black, the color we will use for the borders of our annotations.
*/
const AIRGBColor BORDER_COLOR = {0, 0, 0};

/** Blue color for rectangle preview
*/
const AIRGBColor PREVIEW_COLOR = {0, 30000, 65000};

/** Light gray background for text (similar to standard Illustrator tools)
*/
const AIRGBColor TEXT_BACKGROUND = {58000, 58000, 58000};

/*
*/
Charts::Charts(): fArtLastHit(NULL), fArtUpdate(false), fCursorUpdate(false), fIsDrawingRect(false), fCurrentChartType(kChartTypeBar), fPluginGroupHandle(NULL)
{
	this->Init(fCursorViewPoint);
	this->Init(fCursorArtPoint);
	this->Init(fArtRect);
	this->Init(fCursorRect);
	this->Init(fArtArtworkBounds);
	this->Init(fRectStartPoint);
	this->Init(fRectEndPoint);
	this->Init(fSnappedPoint);
}

/*
*/
void Charts::Init(AIPoint& point)
{
	point.h = 0, point.v = 0;
}

/*
*/
void Charts::Init(AIRealPoint& point)
{
	point.h = 0, point.v = 0;
}

/*
*/
void Charts::Init(AIRect& rect)
{
	rect.left = 0; rect.top = 0; rect.right = 0; rect.bottom = 0;
}

/*
*/
void Charts::Init(AIRealRect& rect)
{
	rect.left = 0; rect.top = 0; rect.right = 0; rect.bottom = 0;
}

/*
*/
ASErr Charts::TrackCursor(AIToolMessage* message)
{
	ASErr result = kNoErr;
	try {
		// Get current view
		AIDocumentViewHandle vh = NULL;
		result = sAIDocumentView->GetNthDocumentView(0, &vh);
		aisdk::check_ai_error(result);
		
		// Update cursor point
		this->fCursorArtPoint = message->cursor;
		
		// Check if smart guides are active and get snap point
		if(sAICursorSnap && sAICursorSnap->UseSmartGuides(vh)) {
			// Use smart guides to get snapped point
			// Control letters: A=anchor, T=path, F=fill, G=gap, P=page, L=artboard, M=art, v=view, i=intersect, o=object, f=frame
			result = sAICursorSnap->Track(vh, fCursorArtPoint, message->event, "ATFGPLM v i o f", &fSnappedPoint);
			if (result == kNoErr) {
				// If dragging, update the end point with snapped position
				if (fIsDrawingRect) {
					fRectEndPoint = fSnappedPoint;
				}
			}
		} else {
			// No smart guides, use raw cursor position
			fSnappedPoint = fCursorArtPoint;
			if (fIsDrawingRect) {
				fRectEndPoint = fCursorArtPoint;
			}
		}
		
		// Convert to view coordinates for display
		result = sAIDocumentView->ArtworkPointToViewPoint(vh, &fCursorArtPoint, &fCursorViewPoint);
		aisdk::check_ai_error(result);
		
		// If drawing, invalidate to update preview
		if (fIsDrawingRect) {
			// Invalidate entire view for simplicity
			AIRealRect viewBounds = {0, 0, 0, 0};
			result = sAIDocumentView->GetDocumentViewBounds(vh, &viewBounds);
			aisdk::check_ai_error(result);
			result = this->InvalidateRect(viewBounds);
			aisdk::check_ai_error(result);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	return result;
}

/*
*/
ASErr Charts::MouseDown(AIToolMessage* message)
{
	ASErr result = kNoErr;
	try {
		// Get current view
		AIDocumentViewHandle vh = NULL;
		result = sAIDocumentView->GetNthDocumentView(0, &vh);
		aisdk::check_ai_error(result);
		
		// Start drawing rectangle
		fIsDrawingRect = true;
		
		// Use smart guides if available
		if(sAICursorSnap && sAICursorSnap->UseSmartGuides(vh)) {
			AIRealPoint snappedStart;
			result = sAICursorSnap->Track(vh, message->cursor, message->event, "ATFGPLM v i o f", &snappedStart);
			if (result == kNoErr) {
				fRectStartPoint = snappedStart;
			} else {
				fRectStartPoint = message->cursor;
			}
		} else {
			fRectStartPoint = message->cursor;
		}
		
		fRectEndPoint = fRectStartPoint;
		
		// Clear any previous art selection
		if (sAIMatchingArt->IsSomeArtSelected) {
			result = sAIMatchingArt->DeselectAll();
			aisdk::check_ai_error(result);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	return result;
}

/*
*/
ASErr Charts::MouseDrag(AIToolMessage* message)
{
	ASErr result = kNoErr;
	try {
		if (fIsDrawingRect) {
			// TrackCursor will handle updating the end point with smart guides
			result = this->TrackCursor(message);
			aisdk::check_ai_error(result);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	return result;
}

/*
*/
ASErr Charts::MouseUp(AIToolMessage* message)
{
	ASErr result = kNoErr;
	try {
		// FIRST: Stop drawing immediately to prevent cursor sticking
		bool wasDrawing = fIsDrawingRect;
		fIsDrawingRect = false;
		
		// Get current view
		AIDocumentViewHandle vh = NULL;
		result = sAIDocumentView->GetNthDocumentView(0, &vh);
		aisdk::check_ai_error(result);
		
		// Invalidate to clear preview immediately
		AIRealRect viewBounds = {0, 0, 0, 0};
		result = sAIDocumentView->GetDocumentViewBounds(vh, &viewBounds);
		aisdk::check_ai_error(result);
		result = this->InvalidateRect(viewBounds);
		aisdk::check_ai_error(result);
		
		if (wasDrawing) {
			// Use smart guides for final position if available
			if(sAICursorSnap && sAICursorSnap->UseSmartGuides(vh)) {
				AIRealPoint snappedEnd;
				result = sAICursorSnap->Track(vh, message->cursor, message->event, "ATFGPLM v i o f", &snappedEnd);
				if (result == kNoErr) {
					fRectEndPoint = snappedEnd;
				} else {
					fRectEndPoint = message->cursor;
				}
			} else {
				fRectEndPoint = message->cursor;
			}
			
			// Create the chart if the rectangle has some size
			if (fRectStartPoint.h != fRectEndPoint.h || fRectStartPoint.v != fRectEndPoint.v) {
				// Calculate chart bounds
				AIRealRect chartBounds;
				chartBounds.left = min(fRectStartPoint.h, fRectEndPoint.h);
				chartBounds.right = max(fRectStartPoint.h, fRectEndPoint.h);
				chartBounds.top = max(fRectStartPoint.v, fRectEndPoint.v);
				chartBounds.bottom = min(fRectStartPoint.v, fRectEndPoint.v);
				
				// Create a Chart object (custom group with chart properties)
				AIArtHandle chartArt = nullptr;
				result = ChartItem::CreatePluginArt(chartBounds, fCurrentChartType, nullptr, &chartArt);
				if (result == kNoErr && chartArt) {
					// Select the new chart
					result = sAIArt->SetArtUserAttr(chartArt, kArtSelected, kArtSelected);
					aisdk::check_ai_error(result);
				} else {
					// Chart creation failed, fall back to simple rectangle
					AIArtHandle rectangleArt;
					result = sAIArt->NewArt(kPathArt, kPlaceAboveAll, nullptr, &rectangleArt);
					aisdk::check_ai_error(result);
					
					// Create simple rectangle
					result = sAIPath->SetPathSegmentCount(rectangleArt, 4);
					aisdk::check_ai_error(result);
					
					AIPathSegment segments[4];
					segments[0].p.h = chartBounds.left; segments[0].p.v = chartBounds.top;
					segments[1].p.h = chartBounds.right; segments[1].p.v = chartBounds.top;
					segments[2].p.h = chartBounds.right; segments[2].p.v = chartBounds.bottom;
					segments[3].p.h = chartBounds.left; segments[3].p.v = chartBounds.bottom;
					
					for (int i = 0; i < 4; i++) {
						segments[i].in = segments[i].out = segments[i].p;
						segments[i].corner = true;
						result = sAIPath->SetPathSegments(rectangleArt, i, 1, &segments[i]);
						aisdk::check_ai_error(result);
					}
					
					result = sAIPath->SetPathClosed(rectangleArt, true);
					aisdk::check_ai_error(result);
					
					// Select the rectangle
					result = sAIArt->SetArtUserAttr(rectangleArt, kArtSelected, kArtSelected);
					aisdk::check_ai_error(result);
				}
			}
		}
	}
	catch (ai::Error& ex) {
		result = ex;
		// Ensure drawing state is reset even on error
		fIsDrawingRect = false;
	}
	return result;
}

/*
*/
ASErr Charts::CheckForArtHit(AIToolMessage* message)
{
	ASErr result = kNoErr;
	try {
		// Only check for art hit when not drawing
		if (!fIsDrawingRect) {
			// Set cursor location.
			this->fCursorArtPoint = message->cursor;
			SDK_ASSERT(sAIDocumentView);
			result = sAIDocumentView->ArtworkPointToViewPoint(NULL, &fCursorArtPoint, &fCursorViewPoint);
			aisdk::check_ai_error(result);

			// Check if cursor is over any art.
			AIHitRef hitRef = NULL;
			AIToolHitData toolHitData;
			SDK_ASSERT(sAIHitTest);
			result = sAIHitTest->HitTest(NULL, &fCursorArtPoint, kAllHitRequest, &hitRef);
			aisdk::check_ai_error(result);
			result = sAIHitTest->GetHitData(hitRef, &toolHitData);
			aisdk::check_ai_error(result);
			result = sAIHitTest->Release(hitRef);
			aisdk::check_ai_error(result);
			
			if (toolHitData.hit && toolHitData.object != NULL && toolHitData.object != fArtLastHit) {
				// New art item hit, set last art object hit in class to art item hit.
				fArtLastHit = toolHitData.object;
				fArtUpdate = true;
				fCursorUpdate = true;

				// Get the art bounds in artwork coordinates.
				SDK_ASSERT(sAIArt);
				result = sAIArt->GetArtTransformBounds(fArtLastHit, NULL, kNoStrokeBounds, &fArtArtworkBounds);
				aisdk::check_ai_error(result);

				AIPoint viewTopRight{ 0, 0 };
				AIRealPoint artworkTopRight{ this->fArtArtworkBounds.right, this->fArtArtworkBounds.top };
				SDK_ASSERT(sAIDocumentView);
				result = sAIDocumentView->ArtworkPointToViewPoint(NULL, &artworkTopRight, &viewTopRight);
				aisdk::check_ai_error(result);

				// Set the annotation location to the right of the art.
				fArtRect.left = viewTopRight.h + 10;
				fArtRect.top = viewTopRight.v;
				fArtRect.right = fArtRect.left + 80;
				fArtRect.bottom = fArtRect.top + 70;
				
				// If other art is selected, deselect all art.
				SDK_ASSERT(sAIMatchingArt);
				if (sAIMatchingArt->IsSomeArtSelected) {
					result = sAIMatchingArt->DeselectAll();
					aisdk::check_ai_error(result); 
				}

				// Select art hit.
				result = sAIArt->SetArtUserAttr(fArtLastHit, kArtSelected, kArtSelected);
				aisdk::check_ai_error(result);
			}
			else if (fArtLastHit != NULL && toolHitData.object == fArtLastHit) {
				fCursorUpdate = true;
				result = this->InvalidateRect(fCursorRect);
				aisdk::check_ai_error(result);
			}
			else {
				// No art hit, deselect all art.
				SDK_ASSERT(sAIMatchingArt);
				result = sAIMatchingArt->DeselectAll();
				aisdk::check_ai_error(result);
				fArtLastHit = NULL;
				fArtUpdate = false;
				fCursorUpdate = false;
			}
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	return result;
}

/*
*/
ASErr Charts::DrawRectanglePreview(AIAnnotatorMessage* message)
{
	ASErr result = kNoErr;
	try {
		if (fIsDrawingRect) {
			// Convert artwork coordinates to view coordinates
			AIPoint startView, endView;
			result = sAIDocumentView->ArtworkPointToViewPoint(NULL, &fRectStartPoint, &startView);
			aisdk::check_ai_error(result);
			result = sAIDocumentView->ArtworkPointToViewPoint(NULL, &fRectEndPoint, &endView);
			aisdk::check_ai_error(result);
			
			// Create view rectangle
			AIRect previewRect;
			previewRect.left = min(startView.h, endView.h);
			previewRect.right = max(startView.h, endView.h);
			previewRect.top = min(startView.v, endView.v);
			previewRect.bottom = max(startView.v, endView.v);
			
			// Draw preview rectangle with solid blue lines
			SDK_ASSERT(sAIAnnotatorDrawer);
			sAIAnnotatorDrawer->SetColor(message->drawer, PREVIEW_COLOR);
			sAIAnnotatorDrawer->SetLineWidth(message->drawer, 2.0);
			sAIAnnotatorDrawer->SetLineDashed(message->drawer, false);
			result = sAIAnnotatorDrawer->DrawRect(message->drawer, previewRect, false);
			aisdk::check_ai_error(result);
			
			// Draw dimensions text at end of drag
			AIReal width = fabs(fRectEndPoint.h - fRectStartPoint.h);
			AIReal height = fabs(fRectEndPoint.v - fRectStartPoint.v);
			
			ai::UnicodeString dimStr;
			ai::NumberFormat numFormat;
			ai::UnicodeString widthStr, heightStr;
			numFormat.toString(width, 2, widthStr);
			numFormat.toString(height, 2, heightStr);
			dimStr = ai::UnicodeString("W: ").append(widthStr).append(ai::UnicodeString("  H: ")).append(heightStr);
			
			// Draw dimension text near end point with background
			AIPoint textPos;
			textPos.h = endView.h + 10;
			textPos.v = endView.v + 20;
			
			// Get text bounds for background
			AIRect textBounds;
			result = sAIAnnotatorDrawer->SetFontPreset(message->drawer, kAIAFMedium);
			aisdk::check_ai_error(result);
			result = sAIAnnotatorDrawer->GetTextBounds(message->drawer, dimStr, &textPos, false, textBounds, false);
			aisdk::check_ai_error(result);
			
			// Expand bounds for padding (more at top)
			textBounds.left -= 6;
			textBounds.right += 6;
			textBounds.top -= 5;
			textBounds.bottom += 3;
			
			// Draw light gray background
			sAIAnnotatorDrawer->SetColor(message->drawer, TEXT_BACKGROUND);
			result = sAIAnnotatorDrawer->DrawRect(message->drawer, textBounds, true);
			aisdk::check_ai_error(result);
			
			// Draw text
			sAIAnnotatorDrawer->SetColor(message->drawer, BORDER_COLOR);
			result = sAIAnnotatorDrawer->DrawText(message->drawer, dimStr, textPos, false);
			aisdk::check_ai_error(result);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	return result;
}

/*
*/
ASErr Charts::DrawArtAnnotation(AIAnnotatorMessage* message)
{
	ASErr result = kNoErr;
	try {
		// Only draw art annotation when not drawing rectangle
		if (!fIsDrawingRect && fArtUpdate) {

			// Draw a light yellow filled rectangle
			SDK_ASSERT(sAIAnnotatorDrawer);
			sAIAnnotatorDrawer->SetColor(message->drawer, BKGND_COLOR);
			result = sAIAnnotatorDrawer->DrawRect(message->drawer, fArtRect, true);
			aisdk::check_ai_error(result);

			// Draw a black border
			sAIAnnotatorDrawer->SetColor(message->drawer, BORDER_COLOR);
			sAIAnnotatorDrawer->SetLineWidth(message->drawer, 0.5);
			result = sAIAnnotatorDrawer->DrawRect(message->drawer, fArtRect, false);
			aisdk::check_ai_error(result);

			// Draw art bounds text content.
			result = sAIAnnotatorDrawer->SetFontPreset(message->drawer, kAIAFSmall);
			aisdk::check_ai_error(result);

			ai::UnicodeString topString, leftString, bottomString, rightString;
			result = this->GetRectString(this->fArtArtworkBounds, topString, leftString, bottomString, rightString);
			aisdk::check_ai_error(result);
			
			AIReal textPadding = sAIAnnotatorDrawer->GetFontSize(message->drawer) + 5;
			AIPoint textBottomLeft;
			textBottomLeft.h = fArtRect.left + 5;
			textBottomLeft.v = fArtRect.top + textPadding;

			// Add art top bounds text.
			result = sAIAnnotatorDrawer->DrawText(message->drawer, topString, textBottomLeft, false);
			aisdk::check_ai_error(result);

			// Add art left bounds text.
			textBottomLeft.v = textBottomLeft.v + textPadding;
			result = sAIAnnotatorDrawer->DrawText(message->drawer, leftString, textBottomLeft, false);
			aisdk::check_ai_error(result);

			// Add art bottom bounds text.
			textBottomLeft.v = textBottomLeft.v + textPadding;
			result = sAIAnnotatorDrawer->DrawText(message->drawer, bottomString, textBottomLeft, false);
			aisdk::check_ai_error(result);

			// Add art right bounds text.
			textBottomLeft.v = textBottomLeft.v + textPadding;
			result = sAIAnnotatorDrawer->DrawText(message->drawer, rightString, textBottomLeft, false);
			aisdk::check_ai_error(result);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	return result;
}

/*
*/
ASErr Charts::DrawCursorAnnotation(AIAnnotatorMessage* message)
{
	ASErr result = kNoErr;
	try {	
		// Only show cursor coordinates when not drawing
		if (!fIsDrawingRect && fCursorUpdate) {
			// Get cursor location as string.
			ai::UnicodeString pointStr;
			result = this->GetPointString(fCursorArtPoint, pointStr);
			aisdk::check_ai_error(result);

			// Get a point a little above and to the right of the cursor location
			ai::int32 offset = 5;
			AIPoint offsetCursorPoint;
			offsetCursorPoint.h = fCursorViewPoint.h + offset;
			offsetCursorPoint.v = fCursorViewPoint.v - offset;

			// Get a rectangle near the cursor representing the dimensions of our text
			AIRect textBounds;
			result = sAIAnnotatorDrawer->GetTextBounds(message->drawer, pointStr, &offsetCursorPoint, false, textBounds, false);
			aisdk::check_ai_error(result);

			// Now construct the rectangle we wish to draw, adding space above and to the right for padding
			fCursorRect = textBounds;
			fCursorRect.top -= 5;
			fCursorRect.right += 20;
			
			// Draw a filled rectangle with a light yellow background color
			sAIAnnotatorDrawer->SetColor(message->drawer, BKGND_COLOR);
			result = sAIAnnotatorDrawer->DrawRect(message->drawer, fCursorRect, true);
			aisdk::check_ai_error(result);

			// Draw a black border around the rectangle
			sAIAnnotatorDrawer->SetColor(message->drawer, BORDER_COLOR);
			sAIAnnotatorDrawer->SetLineWidth(message->drawer, 0.5);
			result = sAIAnnotatorDrawer->DrawRect(message->drawer, fCursorRect, false);
			aisdk::check_ai_error(result);

			// Add the text to the rectangle
			result = sAIAnnotatorDrawer->SetFontPreset(message->drawer, kAIAFSmall);
			aisdk::check_ai_error(result);
			result = sAIAnnotatorDrawer->DrawTextAligned(message->drawer, pointStr, kAICenter, kAIMiddle, fCursorRect, false);
			aisdk::check_ai_error(result);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	return result;
}

/*
*/
ASErr Charts::InvalidateRect(const AIRealRect& invalRealRect)
{
	ASErr result = kNoErr;
	try {		
		// invalRealRect is in artwork coordinates, convert to view 
		// coordinates for AIAnnotatorSuite::InvalAnnotationRect.
		AIRect invalRect;
		result = this->ArtworkBoundsToViewBounds(invalRealRect, invalRect);
		aisdk::check_ai_error(result);

		// Invalidate the rect bounds so it is redrawn.
		SDK_ASSERT(sAIAnnotator);
		result = sAIAnnotator->InvalAnnotationRect(NULL, &invalRect);
		aisdk::check_ai_error(result);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	return result;
}

/*
*/
ASErr Charts::InvalidateRect(const AIRect& invalRect)
{
	ASErr result = kNoErr;
	try {		
		// Invalidate the rect bounds so it is redrawn.
		SDK_ASSERT(sAIAnnotator);
		result = sAIAnnotator->InvalAnnotationRect(NULL, &invalRect);
		aisdk::check_ai_error(result);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	return result;
}

/*
*/
ASErr Charts::ArtworkBoundsToViewBounds(const AIRealRect& artworkBounds, AIRect& viewBounds)
{
	ASErr result = kNoErr;
	try {		
		SDK_ASSERT(sAIDocumentView);
		result = sAIDocumentView->ArtworkRectToViewRect(NULL, &artworkBounds, &viewBounds);
		aisdk::check_ai_error(result);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	return result;
}

ASErr Charts::ArtworkBoundsToViewBoundsUnrotated(const AIRealRect& artworkBounds, AIRect& viewBounds)
{
	ASErr result = kNoErr;
	try {
		SDK_ASSERT(sAIDocumentView);
		result = sAIDocumentView->ArtworkRectToViewRectUnrotated(NULL, &artworkBounds, &viewBounds);
		aisdk::check_ai_error(result);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	return result;
}

/*
*/
ASErr Charts::GetRectString(const AIRealRect& rect, ai::UnicodeString& topString,
							   ai::UnicodeString& leftString, ai::UnicodeString& bottomString,
							   ai::UnicodeString& rightString)
{
	ASErr result = kNoErr;
	try
	{		
		ai::int32 precision = 2;
		ai::UnicodeString top, left, bottom, right;
		ai::NumberFormat numFormat;
		top = numFormat.toString(-rect.top, precision, top, true);
		left = numFormat.toString(rect.left, precision, left, true);
		bottom = numFormat.toString(-rect.bottom, precision, bottom, true);
		right = numFormat.toString(rect.right, precision, right, true);
		topString = ai::UnicodeString("Top: ").append(top);
		leftString = ai::UnicodeString("Left: ").append(left);
		bottomString = ai::UnicodeString("Bottom: ").append(bottom);
		rightString = ai::UnicodeString("Right: ").append(right);
	}
	catch (ai::Error& ex)
	{
		result = ex;
	}

	return result;
}

/*
*/
ASErr Charts::GetPointString(const AIRealPoint& point, ai::UnicodeString& pointStr)
{
	ASErr result = kNoErr;
	try
	{		
		ai::int32 precision = 2;
		ai::UnicodeString horiz, vert;
		ai::NumberFormat numFormat;
		horiz = numFormat.toString(point.h, precision, horiz);
		vert = numFormat.toString(-point.v, precision, vert);
		pointStr = pointStr.append(ai::UnicodeString("x: ").append(horiz).append(ai::UnicodeString(", y: ").append(vert)));
	}
	catch (ai::Error& ex)
	{
		result = ex;
	}

	return result;
}

/*
*/
void Charts::SetPluginGroupHandle(AIPluginGroupHandle handle)
{
	fPluginGroupHandle = handle;
}
