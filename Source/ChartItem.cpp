//========================================================================================
//  
//  ChartItem.cpp
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
#include "ChartItem.h"
#include "ChartsSuites.h"
#include "SDKErrors.h"
#include "AIPluginGroup.h"
#include "AITextFrame.h"
#include "IText.h"

// Initialize static member
ai::int32 ChartItem::sNextChartID = 1;

/*
*/
ChartItem::ChartItem() : 
	fChartType(kChartTypeBar),
	fShowLegend(true),
	fShowGrid(true),
	fShowDataLabels(false),
	fMargin(20.0),
	fChartGroup(nullptr),
	fChartID(sNextChartID++)
{
	// Initialize bounds to zero
	fBounds.left = 0;
	fBounds.top = 0;
	fBounds.right = 0;
	fBounds.bottom = 0;
	
	// Create a default data series
	ChartDataSeries defaultSeries;
	defaultSeries.name = ai::UnicodeString("Series 1");
	fDataSeries.push_back(defaultSeries);
}

/*
*/
ChartItem::ChartItem(const AIRealRect& bounds, ChartType type) : 
	fBounds(bounds),
	fChartType(type),
	fShowLegend(true),
	fShowGrid(true),
	fShowDataLabels(false),
	fMargin(20.0),
	fChartGroup(nullptr),
	fChartID(sNextChartID++)
{
	// Create a default data series
	ChartDataSeries defaultSeries;
	defaultSeries.name = ai::UnicodeString("Series 1");
	fDataSeries.push_back(defaultSeries);
}

/*
*/
ChartItem::~ChartItem()
{
	// Clean up if needed
	if (fChartGroup) {
		// Note: We don't delete the art here as it should be managed by Illustrator
		// This is just a reference
	}
}

/*
*/
void ChartItem::AddDataSeries(const ChartDataSeries& series)
{
	fDataSeries.push_back(series);
}

/*
*/
void ChartItem::ClearData()
{
	fDataSeries.clear();
}

/*
*/
ChartDataSeries* ChartItem::GetSeries(size_t index)
{
	if (index < fDataSeries.size()) {
		return &fDataSeries[index];
	}
	return nullptr;
}

/*
*/
const ChartDataSeries* ChartItem::GetSeries(size_t index) const
{
	if (index < fDataSeries.size()) {
		return &fDataSeries[index];
	}
	return nullptr;
}

/*
*/
void ChartItem::AddDataPoint(AIReal value, const ai::UnicodeString& label)
{
	// Add to the first series (or create one if none exists)
	if (fDataSeries.empty()) {
		ChartDataSeries series;
		series.name = ai::UnicodeString("Series 1");
		fDataSeries.push_back(series);
	}
	
	ChartDataPoint point(value, label);
	fDataSeries[0].dataPoints.push_back(point);
}

/*
*/
void ChartItem::AddDataPoint(const ChartDataPoint& point)
{
	// Add to the first series (or create one if none exists)
	if (fDataSeries.empty()) {
		ChartDataSeries series;
		series.name = ai::UnicodeString("Series 1");
		fDataSeries.push_back(series);
	}
	
	fDataSeries[0].dataPoints.push_back(point);
}

/*
*/
ASErr ChartItem::CreateChartArt()
{
	ASErr result = kNoErr;
	
	try {
		// First delete any existing art
		if (fChartGroup) {
			result = DeleteChartArt();
			aisdk::check_ai_error(result);
		}
		
		// Create a regular group for the chart (we'll add custom data via dictionary)
		result = sAIArt->NewArt(kGroupArt, kPlaceAboveAll, nullptr, &fChartGroup);
		aisdk::check_ai_error(result);
		
		// Get the dictionary for this art object (create one if it doesn't exist)
		AIDictionaryRef dict = nullptr;
		result = sAIArt->GetDictionary(fChartGroup, &dict);
		if (result != kNoErr || !dict) {
			// If no dictionary exists, create one
			result = sAIDictionary->CreateDictionary(&dict);
			aisdk::check_ai_error(result);
		}
		
		// Write chart data to dictionary
		result = WriteToDictionary(dict);
		aisdk::check_ai_error(result);
		
		// Set the art name to identify it as a chart
		ai::UnicodeString artName("Chart");
		result = sAIArt->SetArtName(fChartGroup, artName);
		aisdk::check_ai_error(result);
		
		// Create the chart background
		result = CreateChartBackground();
		aisdk::check_ai_error(result);
		
		// Render the chart based on type
		switch (fChartType) {
			case kChartTypeBar:
				result = RenderBarChart();
				break;
			case kChartTypeLine:
				result = RenderLineChart();
				break;
			case kChartTypePie:
				result = RenderPieChart();
				break;
			case kChartTypeArea:
				result = RenderAreaChart();
				break;
			case kChartTypeScatter:
				result = RenderScatterChart();
				break;
			case kChartTypeColumn:
				result = RenderColumnChart();
				break;
			case kChartTypeDonut:
				result = RenderDonutChart();
				break;
			case kChartTypeRadar:
				result = RenderRadarChart();
				break;
			default:
				// Default to bar chart
				result = RenderBarChart();
				break;
		}
		aisdk::check_ai_error(result);
		
		// Create axes if applicable
		if (fChartType != kChartTypePie && fChartType != kChartTypeDonut) {
			result = CreateAxes();
			aisdk::check_ai_error(result);
		}
		
		// Create legend if enabled
		if (fShowLegend && fDataSeries.size() > 1) {
			result = CreateLegend();
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
ASErr ChartItem::UpdateChartArt()
{
	// Delete existing art and recreate
	return CreateChartArt();
}

/*
*/
ASErr ChartItem::DeleteChartArt()
{
	ASErr result = kNoErr;
	
	if (fChartGroup) {
		result = sAIArt->DisposeArt(fChartGroup);
		fChartGroup = nullptr;
	}
	
	return result;
}

/*
*/
AIBoolean ChartItem::ValidateData() const
{
	// Check if we have at least one series with data
	if (fDataSeries.empty()) {
		return false;
	}
	
	for (const auto& series : fDataSeries) {
		if (series.dataPoints.empty()) {
			return false;
		}
	}
	
	return true;
}

/*
*/
ai::UnicodeString ChartItem::GetChartTypeString() const
{
	switch (fChartType) {
		case kChartTypeBar:
			return ai::UnicodeString("Bar Chart");
		case kChartTypeLine:
			return ai::UnicodeString("Line Chart");
		case kChartTypePie:
			return ai::UnicodeString("Pie Chart");
		case kChartTypeArea:
			return ai::UnicodeString("Area Chart");
		case kChartTypeScatter:
			return ai::UnicodeString("Scatter Plot");
		case kChartTypeColumn:
			return ai::UnicodeString("Column Chart");
		case kChartTypeDonut:
			return ai::UnicodeString("Donut Chart");
		case kChartTypeRadar:
			return ai::UnicodeString("Radar Chart");
		default:
			return ai::UnicodeString("Unknown Chart");
	}
}

/*
*/
ASErr ChartItem::CreateChartBackground()
{
	ASErr result = kNoErr;
	
	try {
		// Create a rectangle for the background
		AIArtHandle backgroundRect;
		result = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, fChartGroup, &backgroundRect);
		aisdk::check_ai_error(result);
		
		// Set up the path
		result = sAIPath->SetPathSegmentCount(backgroundRect, 4);
		aisdk::check_ai_error(result);
		
		AIPathSegment segments[4];
		
		// Top-left corner
		segments[0].p.h = fBounds.left;
		segments[0].p.v = fBounds.top;
		segments[0].in = segments[0].out = segments[0].p;
		segments[0].corner = true;
		
		// Top-right corner
		segments[1].p.h = fBounds.right;
		segments[1].p.v = fBounds.top;
		segments[1].in = segments[1].out = segments[1].p;
		segments[1].corner = true;
		
		// Bottom-right corner
		segments[2].p.h = fBounds.right;
		segments[2].p.v = fBounds.bottom;
		segments[2].in = segments[2].out = segments[2].p;
		segments[2].corner = true;
		
		// Bottom-left corner
		segments[3].p.h = fBounds.left;
		segments[3].p.v = fBounds.bottom;
		segments[3].in = segments[3].out = segments[3].p;
		segments[3].corner = true;
		
		for (ai::int32 i = 0; i < 4; i++) {
			result = sAIPath->SetPathSegments(backgroundRect, i, 1, &segments[i]);
			aisdk::check_ai_error(result);
		}
		
		result = sAIPath->SetPathClosed(backgroundRect, true);
		aisdk::check_ai_error(result);
		
		// Set the style - white fill with light gray stroke
		AIPathStyle style;
		AIBoolean hasAdvFill = false;
		result = sAIPathStyle->GetPathStyle(backgroundRect, &style, &hasAdvFill);
		aisdk::check_ai_error(result);
		
		// White fill
		style.fillPaint = true;
		style.fill.color.kind = kGrayColor;
		style.fill.color.c.g.gray = 0;  // White
		
		// Light gray stroke
		style.strokePaint = true;
		style.stroke.color.kind = kGrayColor;
		style.stroke.color.c.g.gray = 0.7 * kAIRealOne;  // Light gray
		style.stroke.width = 1.0;
		
		result = sAIPathStyle->SetPathStyle(backgroundRect, &style);
		aisdk::check_ai_error(result);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	
	return result;
}

/*
*/
ASErr ChartItem::CreateAxes()
{
	ASErr result = kNoErr;
	
	// TODO: Implement axes creation
	// This will draw X and Y axes with tick marks and labels
	
	return result;
}

/*
*/
ASErr ChartItem::CreateLegend()
{
	ASErr result = kNoErr;
	
	// TODO: Implement legend creation
	// This will create a legend showing series names and colors
	
	return result;
}

/*
*/
void ChartItem::CalculateDataRange(AIReal& minValue, AIReal& maxValue) const
{
	minValue = 0;
	maxValue = 0;
	
	bool firstValue = true;
	
	for (const auto& series : fDataSeries) {
		for (const auto& point : series.dataPoints) {
			if (firstValue) {
				minValue = maxValue = point.value;
				firstValue = false;
			} else {
				if (point.value < minValue) minValue = point.value;
				if (point.value > maxValue) maxValue = point.value;
			}
		}
	}
	
	// Add some padding to the range
	AIReal range = maxValue - minValue;
	if (range > 0) {
		minValue -= range * 0.1;
		maxValue += range * 0.1;
	}
	
	// Ensure we include zero for bar/column charts
	if (fChartType == kChartTypeBar || fChartType == kChartTypeColumn) {
		if (minValue > 0) minValue = 0;
		if (maxValue < 0) maxValue = 0;
	}
}

/*
*/
ASErr ChartItem::RenderBarChart()
{
	ASErr result = kNoErr;
	
	try {
		// Simple bar chart implementation for now
		// TODO: Full implementation with proper scaling and multiple series support
		
		if (!ValidateData()) {
			return kBadParameterErr;
		}
		
		// Get the first series
		const ChartDataSeries* series = GetSeries(0);
		if (!series || series->dataPoints.empty()) {
			return kBadParameterErr;
		}
		
		// Calculate chart area (inside margins)
		AIRealRect chartArea;
		chartArea.left = fBounds.left + fMargin;
		chartArea.right = fBounds.right - fMargin;
		chartArea.top = fBounds.top + fMargin;
		chartArea.bottom = fBounds.bottom - fMargin;
		
		AIReal chartWidth = chartArea.right - chartArea.left;
		AIReal chartHeight = chartArea.bottom - chartArea.top;
		
		// Calculate bar width
		size_t numBars = series->dataPoints.size();
		AIReal barWidth = chartWidth / (numBars * 1.5);  // Leave space between bars
		AIReal barSpacing = barWidth * 0.5;
		
		// Find data range
		AIReal minValue, maxValue;
		CalculateDataRange(minValue, maxValue);
		
		// Draw bars
		for (size_t i = 0; i < numBars; i++) {
			const ChartDataPoint& point = series->dataPoints[i];
			
			// Calculate bar position and height
			AIReal barX = chartArea.left + (i * (barWidth + barSpacing)) + barSpacing/2;
			AIReal barHeight = (point.value / maxValue) * chartHeight;
			
			// Create bar rectangle
			AIArtHandle barArt;
			result = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, fChartGroup, &barArt);
			aisdk::check_ai_error(result);
			
			// Set up the path
			result = sAIPath->SetPathSegmentCount(barArt, 4);
			aisdk::check_ai_error(result);
			
			AIPathSegment segments[4];
			
			// Bottom-left corner
			segments[0].p.h = barX;
			segments[0].p.v = chartArea.bottom;
			segments[0].in = segments[0].out = segments[0].p;
			segments[0].corner = true;
			
			// Top-left corner
			segments[1].p.h = barX;
			segments[1].p.v = chartArea.bottom - barHeight;
			segments[1].in = segments[1].out = segments[1].p;
			segments[1].corner = true;
			
			// Top-right corner
			segments[2].p.h = barX + barWidth;
			segments[2].p.v = chartArea.bottom - barHeight;
			segments[2].in = segments[2].out = segments[2].p;
			segments[2].corner = true;
			
			// Bottom-right corner
			segments[3].p.h = barX + barWidth;
			segments[3].p.v = chartArea.bottom;
			segments[3].in = segments[3].out = segments[3].p;
			segments[3].corner = true;
			
			for (ai::int32 j = 0; j < 4; j++) {
				result = sAIPath->SetPathSegments(barArt, j, 1, &segments[j]);
				aisdk::check_ai_error(result);
			}
			
			result = sAIPath->SetPathClosed(barArt, true);
			aisdk::check_ai_error(result);
			
			// Set the style - blue fill
			AIPathStyle style;
			AIBoolean hasAdvFill = false;
			result = sAIPathStyle->GetPathStyle(barArt, &style, &hasAdvFill);
			aisdk::check_ai_error(result);
			
			// Blue fill
			style.fillPaint = true;
			style.fill.color.kind = kThreeColor;
			style.fill.color.c.rgb.red = 0;
			style.fill.color.c.rgb.green = 30000;
			style.fill.color.c.rgb.blue = 65000;
			
			// No stroke for now
			style.strokePaint = false;
			
			result = sAIPathStyle->SetPathStyle(barArt, &style);
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
ASErr ChartItem::RenderLineChart()
{
	// TODO: Implement line chart rendering
	return kNoErr;
}

/*
*/
ASErr ChartItem::RenderPieChart()
{
	// TODO: Implement pie chart rendering
	return kNoErr;
}

/*
*/
ASErr ChartItem::RenderAreaChart()
{
	// TODO: Implement area chart rendering
	return kNoErr;
}

/*
*/
ASErr ChartItem::RenderScatterChart()
{
	// TODO: Implement scatter chart rendering
	return kNoErr;
}

/*
*/
ASErr ChartItem::RenderColumnChart()
{
	// TODO: Implement column chart rendering (vertical bars)
	// For now, just use bar chart rotated
	return RenderBarChart();
}

/*
*/
ASErr ChartItem::RenderDonutChart()
{
	// TODO: Implement donut chart rendering
	return kNoErr;
}

/*
*/
ASErr ChartItem::RenderRadarChart()
{
	// TODO: Implement radar chart rendering
	return kNoErr;
}

/*
*/
ASErr ChartItem::WriteToDictionary(AIDictionaryRef dict) const
{
	ASErr result = kNoErr;
	
	try {
		// Write chart type
		result = sAIDictionary->SetIntegerEntry(dict, sAIDictionary->Key(kChartTypeDictKey), (ai::int32)fChartType);
		aisdk::check_ai_error(result);
		
		// Write chart ID
		result = sAIDictionary->SetIntegerEntry(dict, sAIDictionary->Key(kChartIDDictKey), fChartID);
		aisdk::check_ai_error(result);
		
		// Write version
		result = sAIDictionary->SetIntegerEntry(dict, sAIDictionary->Key(kChartVersionDictKey), 1);
		aisdk::check_ai_error(result);
		
		// Write chart properties (use as_Platform to get C string)
		result = sAIDictionary->SetStringEntry(dict, sAIDictionary->Key(kChartTitleDictKey), fTitle.as_Platform().c_str());
		aisdk::check_ai_error(result);
		
		result = sAIDictionary->SetStringEntry(dict, sAIDictionary->Key(kChartXAxisLabelDictKey), fXAxisLabel.as_Platform().c_str());
		aisdk::check_ai_error(result);
		
		result = sAIDictionary->SetStringEntry(dict, sAIDictionary->Key(kChartYAxisLabelDictKey), fYAxisLabel.as_Platform().c_str());
		aisdk::check_ai_error(result);
		
		// Write visual properties
		result = sAIDictionary->SetBooleanEntry(dict, sAIDictionary->Key(kChartShowLegendDictKey), fShowLegend);
		aisdk::check_ai_error(result);
		
		result = sAIDictionary->SetBooleanEntry(dict, sAIDictionary->Key(kChartShowGridDictKey), fShowGrid);
		aisdk::check_ai_error(result);
		
		result = sAIDictionary->SetBooleanEntry(dict, sAIDictionary->Key(kChartShowDataLabelsDictKey), fShowDataLabels);
		aisdk::check_ai_error(result);
		
		result = sAIDictionary->SetRealEntry(dict, sAIDictionary->Key(kChartMarginDictKey), fMargin);
		aisdk::check_ai_error(result);
		
		// TODO: Serialize data series to dictionary
		// This would require creating a nested structure or array in the dictionary
		// For now, we'll store the data in the art structure itself
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	
	return result;
}

/*
*/
ASErr ChartItem::ReadFromDictionary(AIDictionaryRef dict)
{
	ASErr result = kNoErr;
	
	try {
		// Read chart type
		ai::int32 chartType;
		result = sAIDictionary->GetIntegerEntry(dict, sAIDictionary->Key(kChartTypeDictKey), &chartType);
		if (result == kNoErr) {
			fChartType = (ChartType)chartType;
		}
		
		// Read chart ID
		result = sAIDictionary->GetIntegerEntry(dict, sAIDictionary->Key(kChartIDDictKey), &fChartID);
		
		// Read chart properties
		const char* titleStr = nullptr;
		result = sAIDictionary->GetStringEntry(dict, sAIDictionary->Key(kChartTitleDictKey), &titleStr);
		if (result == kNoErr && titleStr) {
			fTitle = ai::UnicodeString(titleStr);
		}
		
		const char* xAxisStr = nullptr;
		result = sAIDictionary->GetStringEntry(dict, sAIDictionary->Key(kChartXAxisLabelDictKey), &xAxisStr);
		if (result == kNoErr && xAxisStr) {
			fXAxisLabel = ai::UnicodeString(xAxisStr);
		}
		
		const char* yAxisStr = nullptr;
		result = sAIDictionary->GetStringEntry(dict, sAIDictionary->Key(kChartYAxisLabelDictKey), &yAxisStr);
		if (result == kNoErr && yAxisStr) {
			fYAxisLabel = ai::UnicodeString(yAxisStr);
		}
		
		// Read visual properties
		result = sAIDictionary->GetBooleanEntry(dict, sAIDictionary->Key(kChartShowLegendDictKey), &fShowLegend);
		result = sAIDictionary->GetBooleanEntry(dict, sAIDictionary->Key(kChartShowGridDictKey), &fShowGrid);
		result = sAIDictionary->GetBooleanEntry(dict, sAIDictionary->Key(kChartShowDataLabelsDictKey), &fShowDataLabels);
		result = sAIDictionary->GetRealEntry(dict, sAIDictionary->Key(kChartMarginDictKey), &fMargin);
		
		// TODO: Deserialize data series from dictionary
		
		result = kNoErr;  // Reset error as some entries might not exist
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	
	return result;
}

/*
*/
AIBoolean ChartItem::IsChartArt(AIArtHandle art)
{
	if (!art) return false;
	
	ASErr result = kNoErr;
	AIBoolean isChart = false;
	
	try {
		// Check if it's a group
		short type;
		result = sAIArt->GetArtType(art, &type);
		if (result == kNoErr && type == kGroupArt) {
			// Check if it has our chart type identifier in the dictionary
			AIDictionaryRef dict = nullptr;
			result = sAIArt->GetDictionary(art, &dict);
			if (result == kNoErr && dict) {
				// Check for our chart version key to identify it as a chart
				ai::int32 version = 0;
				result = sAIDictionary->GetIntegerEntry(dict, sAIDictionary->Key(kChartVersionDictKey), &version);
				if (result == kNoErr && version > 0) {
					isChart = true;
				}
			}
		}
	}
	catch (...) {
		isChart = false;
	}
	
	return isChart;
}

/*
*/
ChartItem* ChartItem::CreateFromArt(AIArtHandle art)
{
	if (!IsChartArt(art)) {
		return nullptr;
	}
	
	ChartItem* chart = nullptr;
	ASErr result = kNoErr;
	
	try {
		// Get the dictionary
		AIDictionaryRef dict = nullptr;
		result = sAIArt->GetDictionary(art, &dict);
		if (result == kNoErr && dict) {
			// Get the bounds
			AIRealRect bounds;
			result = sAIArt->GetArtBounds(art, &bounds);
			if (result == kNoErr) {
				// Create a new ChartItem
				chart = new ChartItem(bounds, kChartTypeBar);
				
				// Read data from dictionary
				chart->ReadFromDictionary(dict);
				chart->SetChartGroup(art);
				
				sAIDictionary->Release(dict);
			}
		}
	}
	catch (...) {
		if (chart) {
			delete chart;
			chart = nullptr;
		}
	}
	
	return chart;
}

/*
*/
ASErr ChartItem::RenderChartContent()
{
	ASErr result = kNoErr;
	
	try {
		if (!fChartGroup) {
			return kBadParameterErr;
		}
		
		// Create the chart background
		result = CreateChartBackground();
		aisdk::check_ai_error(result);
		
		// Render the chart based on type
		switch (fChartType) {
			case kChartTypeBar:
				result = RenderBarChart();
				break;
			case kChartTypeLine:
				result = RenderLineChart();
				break;
			case kChartTypePie:
				result = RenderPieChart();
				break;
			case kChartTypeArea:
				result = RenderAreaChart();
				break;
			case kChartTypeScatter:
				result = RenderScatterChart();
				break;
			case kChartTypeColumn:
				result = RenderColumnChart();
				break;
			case kChartTypeDonut:
				result = RenderDonutChart();
				break;
			case kChartTypeRadar:
				result = RenderRadarChart();
				break;
			default:
				// Default to bar chart
				result = RenderBarChart();
				break;
		}
		aisdk::check_ai_error(result);
		
		// Create axes if applicable
		if (fChartType != kChartTypePie && fChartType != kChartTypeDonut) {
			result = CreateAxes();
			aisdk::check_ai_error(result);
		}
		
		// Create legend if enabled
		if (fShowLegend && fDataSeries.size() > 1) {
			result = CreateLegend();
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
ASErr ChartItem::CreatePluginArt(const AIRealRect& bounds, ChartType type, AIPluginGroupHandle pluginGroupHandle, AIArtHandle* chartArt)
{
	ASErr result = kNoErr;
	
	try {
		// Since plugin groups aren't working, create a custom group with identifying properties
		// This will appear as "<Chart>" in the Layers panel
		result = sAIArt->NewArt(kGroupArt, kPlaceAboveAll, nullptr, chartArt);
		if (result != kNoErr) {
			return result;
		}
		
		// Set the art name to identify it as a Chart
		ai::UnicodeString chartName("<Chart>");
		result = sAIArt->SetArtName(*chartArt, chartName);
		if (result != kNoErr) {
			sAIArt->DisposeArt(*chartArt);
			*chartArt = nullptr;
			return result;
		}
		
		// Store chart data in the art's dictionary
		AIDictionaryRef dict = nullptr;
		result = sAIArt->GetDictionary(*chartArt, &dict);
		if (result != kNoErr || !dict) {
			// Groups should have dictionaries automatically
			sAIArt->DisposeArt(*chartArt);
			*chartArt = nullptr;
			return kBadParameterErr;
		}
		
		// Mark this as a Chart in the dictionary
		result = sAIDictionary->SetBooleanEntry(dict, sAIDictionary->Key("isChart"), true);
		result = sAIDictionary->SetIntegerEntry(dict, sAIDictionary->Key("chartType"), (ai::int32)type);
		sAIDictionary->Release(dict);
		
		// Use the group itself as the container for chart content
		AIArtHandle resultArt = *chartArt;
		
		// The bounds passed in ARE the plot area
		// We need to position labels outside this area
		AIRealRect plotArea = bounds;  // The drawn rectangle IS the plot area
		AIReal labelGap = 6.0;  // 6 points gap between plot area and labels
		
		// Create named groups for chart components (in z-order from back to front)
		
		// 1. Background group
		AIArtHandle backgroundGroup;
		result = sAIArt->NewArt(kGroupArt, kPlaceInsideOnTop, resultArt, &backgroundGroup);
		aisdk::check_ai_error(result);
		result = sAIArt->SetArtName(backgroundGroup, ai::UnicodeString("Background"));
		
		// 2. Grid groups
		AIArtHandle xGridGroup;
		result = sAIArt->NewArt(kGroupArt, kPlaceInsideOnTop, resultArt, &xGridGroup);
		aisdk::check_ai_error(result);
		result = sAIArt->SetArtName(xGridGroup, ai::UnicodeString("X Grid"));
		
		AIArtHandle yGridGroup;
		result = sAIArt->NewArt(kGroupArt, kPlaceInsideOnTop, resultArt, &yGridGroup);
		aisdk::check_ai_error(result);
		result = sAIArt->SetArtName(yGridGroup, ai::UnicodeString("Y Grid"));
		
		// 3. Column groups (we'll create multiple as needed)
		AIArtHandle columnsGroup;
		result = sAIArt->NewArt(kGroupArt, kPlaceInsideOnTop, resultArt, &columnsGroup);
		aisdk::check_ai_error(result);
		result = sAIArt->SetArtName(columnsGroup, ai::UnicodeString("Columns"));
		
		// 4. Axis groups
		AIArtHandle xAxisGroup;
		result = sAIArt->NewArt(kGroupArt, kPlaceInsideOnTop, resultArt, &xAxisGroup);
		aisdk::check_ai_error(result);
		result = sAIArt->SetArtName(xAxisGroup, ai::UnicodeString("X Axis"));
		
		AIArtHandle yAxisGroup;
		result = sAIArt->NewArt(kGroupArt, kPlaceInsideOnTop, resultArt, &yAxisGroup);
		aisdk::check_ai_error(result);
		result = sAIArt->SetArtName(yAxisGroup, ai::UnicodeString("Y Axis"));
		
		// 5. Tick groups
		AIArtHandle xTicksGroup;
		result = sAIArt->NewArt(kGroupArt, kPlaceInsideOnTop, resultArt, &xTicksGroup);
		aisdk::check_ai_error(result);
		result = sAIArt->SetArtName(xTicksGroup, ai::UnicodeString("X Ticks"));
		
		AIArtHandle yTicksGroup;
		result = sAIArt->NewArt(kGroupArt, kPlaceInsideOnTop, resultArt, &yTicksGroup);
		aisdk::check_ai_error(result);
		result = sAIArt->SetArtName(yTicksGroup, ai::UnicodeString("Y Ticks"));
		
		// 6. Label groups
		AIArtHandle xLabelsGroup;
		result = sAIArt->NewArt(kGroupArt, kPlaceInsideOnTop, resultArt, &xLabelsGroup);
		aisdk::check_ai_error(result);
		result = sAIArt->SetArtName(xLabelsGroup, ai::UnicodeString("X Axis Labels"));
		
		AIArtHandle yLabelsGroup;
		result = sAIArt->NewArt(kGroupArt, kPlaceInsideOnTop, resultArt, &yLabelsGroup);
		aisdk::check_ai_error(result);
		result = sAIArt->SetArtName(yLabelsGroup, ai::UnicodeString("Y Axis Labels"));
		
		// Create the plot area background in the background group
		AIArtHandle plotAreaRect;
		result = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, backgroundGroup, &plotAreaRect);
		aisdk::check_ai_error(result);
		
		result = sAIPath->SetPathSegmentCount(plotAreaRect, 4);
		aisdk::check_ai_error(result);
		
		AIPathSegment segments[4];
		segments[0].p.h = plotArea.left; segments[0].p.v = plotArea.top;
		segments[1].p.h = plotArea.right; segments[1].p.v = plotArea.top;
		segments[2].p.h = plotArea.right; segments[2].p.v = plotArea.bottom;
		segments[3].p.h = plotArea.left; segments[3].p.v = plotArea.bottom;
		
		for (int i = 0; i < 4; i++) {
			segments[i].in = segments[i].out = segments[i].p;
			segments[i].corner = true;
			result = sAIPath->SetPathSegments(plotAreaRect, i, 1, &segments[i]);
			aisdk::check_ai_error(result);
		}
		
		result = sAIPath->SetPathClosed(plotAreaRect, true);
		aisdk::check_ai_error(result);
		
		// Plot area style - white fill with gray stroke
		AIPathStyle style;
		AIBoolean hasAdvFill = false;
		result = sAIPathStyle->GetPathStyle(plotAreaRect, &style, &hasAdvFill);
		aisdk::check_ai_error(result);
		
		style.fillPaint = true;
		style.fill.color.kind = kGrayColor;
		style.fill.color.c.g.gray = 0;  // White
		
		style.strokePaint = true;
		style.stroke.color.kind = kGrayColor;
		style.stroke.color.c.g.gray = 0.5 * kAIRealOne;  // Medium gray
		style.stroke.width = 0.5;
		
		result = sAIPathStyle->SetPathStyle(plotAreaRect, &style);
		aisdk::check_ai_error(result);
		
		// Calculate plot dimensions
		AIReal plotWidth = plotArea.right - plotArea.left;
		AIReal plotHeight = plotArea.top - plotArea.bottom;
		
		// Create horizontal grid lines (Y grid)
		for (int i = 1; i <= 4; i++) {  // Skip 0 and 100 as they're on the border
			AIArtHandle gridLine;
			result = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, yGridGroup, &gridLine);
			if (result == kNoErr) {
				result = sAIPath->SetPathSegmentCount(gridLine, 2);
				AIPathSegment gridSegs[2];
				gridSegs[0].p.h = plotArea.left;
				gridSegs[0].p.v = plotArea.bottom + (i * plotHeight / 4);
				gridSegs[0].in = gridSegs[0].out = gridSegs[0].p;
				gridSegs[0].corner = true;
				
				gridSegs[1].p.h = plotArea.right;
				gridSegs[1].p.v = plotArea.bottom + (i * plotHeight / 4);
				gridSegs[1].in = gridSegs[1].out = gridSegs[1].p;
				gridSegs[1].corner = true;
				
				for (int j = 0; j < 2; j++) {
					result = sAIPath->SetPathSegments(gridLine, j, 1, &gridSegs[j]);
				}
				result = sAIPath->SetPathClosed(gridLine, false);
				
				// Style the grid line
				AIPathStyle gridStyle;
				AIBoolean hasAdvFill = false;
				sAIPathStyle->GetPathStyle(gridLine, &gridStyle, &hasAdvFill);
				gridStyle.fillPaint = false;
				gridStyle.strokePaint = true;
				gridStyle.stroke.color.kind = kGrayColor;
				gridStyle.stroke.color.c.g.gray = 0.85 * kAIRealOne;  // Light gray
				gridStyle.stroke.width = 0.25;
				gridStyle.stroke.dash.length = 2;  // Number of dash entries
				gridStyle.stroke.dash.array[0] = 2.0;  // Dash length
				gridStyle.stroke.dash.array[1] = 2.0;  // Gap length
				sAIPathStyle->SetPathStyle(gridLine, &gridStyle);
			}
		}
		
		// Create column chart with multiple data series
		const int numCategories = 5;  // Number of X-axis categories (months)
		const int numSeries = 3;      // Number of data series (e.g., different years)
		
		// Multiple data series - each row is a series
		AIReal values[numSeries][numCategories] = {
			{75.0, 45.0, 90.0, 60.0, 85.0},  // Series 1 (e.g., 2022)
			{65.0, 55.0, 80.0, 70.0, 75.0},  // Series 2 (e.g., 2023)
			{85.0, 50.0, 95.0, 65.0, 90.0}   // Series 3 (e.g., 2024)
		};
		
		const char* categoryLabels[numCategories] = {"Jan", "Feb", "Mar", "Apr", "May"};
		const char* seriesNames[numSeries] = {"2022", "2023", "2024"};
		
		// Define colors for each series
		struct SeriesColor {
			AIReal red, green, blue;
		};
		SeriesColor seriesColors[numSeries] = {
			{15000, 35000, 55000},  // Blue for series 1
			{45000, 25000, 15000},  // Orange for series 2
			{25000, 45000, 25000}   // Green for series 3
		};
		
		// Calculate column dimensions
		AIReal categoryWidth = plotWidth / numCategories;
		AIReal columnGroupWidth = categoryWidth * 0.8;  // 80% of category width for columns
		AIReal columnGroupGap = categoryWidth * 0.2;    // 20% for gaps
		AIReal individualColumnWidth = columnGroupWidth / numSeries;
		
		// Create vertical grid lines (X grid) - one per category
		for (int i = 0; i < numCategories; i++) {
			AIReal xPos = plotArea.left + (i + 0.5) * categoryWidth;  // Center of category
			AIArtHandle gridLine;
			result = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, xGridGroup, &gridLine);
			if (result == kNoErr) {
				result = sAIPath->SetPathSegmentCount(gridLine, 2);
				AIPathSegment gridSegs[2];
				gridSegs[0].p.h = xPos;
				gridSegs[0].p.v = plotArea.bottom;
				gridSegs[0].in = gridSegs[0].out = gridSegs[0].p;
				gridSegs[0].corner = true;
				
				gridSegs[1].p.h = xPos;
				gridSegs[1].p.v = plotArea.top;
				gridSegs[1].in = gridSegs[1].out = gridSegs[1].p;
				gridSegs[1].corner = true;
				
				for (int j = 0; j < 2; j++) {
					result = sAIPath->SetPathSegments(gridLine, j, 1, &gridSegs[j]);
				}
				result = sAIPath->SetPathClosed(gridLine, false);
				
				// Style the grid line
				AIPathStyle gridStyle;
				AIBoolean hasAdvFill = false;
				sAIPathStyle->GetPathStyle(gridLine, &gridStyle, &hasAdvFill);
				gridStyle.fillPaint = false;
				gridStyle.strokePaint = true;
				gridStyle.stroke.color.kind = kGrayColor;
				gridStyle.stroke.color.c.g.gray = 0.85 * kAIRealOne;  // Light gray
				gridStyle.stroke.width = 0.25;
				gridStyle.stroke.dash.length = 2;  // Number of dash entries
				gridStyle.stroke.dash.array[0] = 2.0;  // Dash length
				gridStyle.stroke.dash.array[1] = 2.0;  // Gap length
				sAIPathStyle->SetPathStyle(gridLine, &gridStyle);
			}
		}
		
		// Create columns organized by series
		for (int seriesIdx = 0; seriesIdx < numSeries; seriesIdx++) {
			// Create a group for this series within the columns group
			AIArtHandle seriesGroup;
			result = sAIArt->NewArt(kGroupArt, kPlaceInsideOnTop, columnsGroup, &seriesGroup);
			aisdk::check_ai_error(result);
			ai::UnicodeString seriesGroupName("Column Set ");
			seriesGroupName.append(ai::UnicodeString(seriesNames[seriesIdx]));
			result = sAIArt->SetArtName(seriesGroup, seriesGroupName);
			
			// Create columns for this series
			for (int catIdx = 0; catIdx < numCategories; catIdx++) {
				// Calculate column position
				AIReal categoryLeft = plotArea.left + catIdx * categoryWidth;
				AIReal columnGroupLeft = categoryLeft + columnGroupGap / 2;
				AIReal columnLeft = columnGroupLeft + seriesIdx * individualColumnWidth;
				AIReal columnRight = columnLeft + individualColumnWidth * 0.9;  // Small gap between columns
				
				// Calculate column height
				AIReal columnHeight = (values[seriesIdx][catIdx] / 100.0) * plotHeight;
				AIReal columnTop = plotArea.bottom + columnHeight;
				
				// Create column rectangle in the series group
				AIArtHandle column;
				result = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, seriesGroup, &column);
				aisdk::check_ai_error(result);
			
				result = sAIPath->SetPathSegmentCount(column, 4);
				aisdk::check_ai_error(result);
				
				segments[0].p.h = columnLeft; segments[0].p.v = plotArea.bottom;
				segments[1].p.h = columnLeft; segments[1].p.v = columnTop;
				segments[2].p.h = columnRight; segments[2].p.v = columnTop;
				segments[3].p.h = columnRight; segments[3].p.v = plotArea.bottom;
				
				for (int j = 0; j < 4; j++) {
					segments[j].in = segments[j].out = segments[j].p;
					segments[j].corner = true;
					result = sAIPath->SetPathSegments(column, j, 1, &segments[j]);
					aisdk::check_ai_error(result);
				}
				
				result = sAIPath->SetPathClosed(column, true);
				aisdk::check_ai_error(result);
				
				// Column style - use series color
				result = sAIPathStyle->GetPathStyle(column, &style, &hasAdvFill);
				aisdk::check_ai_error(result);
				
				style.fillPaint = true;
				style.fill.color.kind = kThreeColor;
				style.fill.color.c.rgb.red = seriesColors[seriesIdx].red;
				style.fill.color.c.rgb.green = seriesColors[seriesIdx].green;
				style.fill.color.c.rgb.blue = seriesColors[seriesIdx].blue;
				style.strokePaint = true;
				style.stroke.color.kind = kGrayColor;
				style.stroke.color.c.g.gray = 0.3 * kAIRealOne;
				style.stroke.width = 0.5;
				
				result = sAIPathStyle->SetPathStyle(column, &style);
				aisdk::check_ai_error(result);
			}
		}
		
		// Create X-axis labels and ticks (one per category)
		for (int i = 0; i < numCategories; i++) {
			AIReal categoryCenter = plotArea.left + (i + 0.5) * categoryWidth;
			
			// X-axis label
			AITextOrientation orient = kHorizontalTextOrientation;
			AIRealPoint anchor;
			anchor.h = categoryCenter;
			anchor.v = plotArea.bottom - labelGap - 10;
			
			AIArtHandle xLabel = nullptr;
			result = sAITextFrame->NewPointText(kPlaceInsideOnTop, xLabelsGroup, orient, anchor, &xLabel);
			if (result == kNoErr && xLabel) {
				TextRangeRef range = nullptr;
				result = sAITextFrame->GetATETextRange(xLabel, &range);
				if (result == kNoErr && range) {
					ATE::ITextRange textRange(range);
					
					// Set the text content
					textRange.InsertAfter(ai::UnicodeString(categoryLabels[i]).as_ASUnicode().c_str());
					
					// Set paragraph alignment to center
					ATE::IParaFeatures paraFeatures;
					paraFeatures.SetJustification(ATE::kCenterJustify);
					textRange.SetLocalParaFeatures(paraFeatures);
				}
			}
			
			// Also add tick mark
			AIArtHandle tick;
			result = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, xTicksGroup, &tick);
			if (result == kNoErr) {
				result = sAIPath->SetPathSegmentCount(tick, 2);
				AIPathSegment tickSegs[2];
				tickSegs[0].p.h = categoryCenter;
				tickSegs[0].p.v = plotArea.bottom;
				tickSegs[0].in = tickSegs[0].out = tickSegs[0].p;
				tickSegs[0].corner = true;
				
				tickSegs[1].p.h = categoryCenter;
				tickSegs[1].p.v = plotArea.bottom - 5;
				tickSegs[1].in = tickSegs[1].out = tickSegs[1].p;
				tickSegs[1].corner = true;
				
				for (int j = 0; j < 2; j++) {
					result = sAIPath->SetPathSegments(tick, j, 1, &tickSegs[j]);
				}
				result = sAIPath->SetPathClosed(tick, false);
				
				AIPathStyle tickStyle;
				AIBoolean hasAdvFill = false;
				sAIPathStyle->GetPathStyle(tick, &tickStyle, &hasAdvFill);
				tickStyle.fillPaint = false;
				tickStyle.strokePaint = true;
				tickStyle.stroke.color.kind = kGrayColor;
				tickStyle.stroke.color.c.g.gray = 0.3 * kAIRealOne;
				tickStyle.stroke.width = 0.5;
				sAIPathStyle->SetPathStyle(tick, &tickStyle);
			}
		}
		
		// Add Y-axis (vertical line)
	AIArtHandle yAxis;
	result = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, yAxisGroup, &yAxis);
	if (result == kNoErr) {
		result = sAIPath->SetPathSegmentCount(yAxis, 2);
		AIPathSegment axisSegs[2];
		axisSegs[0].p.h = plotArea.left;
		axisSegs[0].p.v = plotArea.bottom;
		axisSegs[0].in = axisSegs[0].out = axisSegs[0].p;
		axisSegs[0].corner = true;
		
		axisSegs[1].p.h = plotArea.left;
		axisSegs[1].p.v = plotArea.top;
		axisSegs[1].in = axisSegs[1].out = axisSegs[1].p;
		axisSegs[1].corner = true;
		
		for (int j = 0; j < 2; j++) {
			result = sAIPath->SetPathSegments(yAxis, j, 1, &axisSegs[j]);
		}
		result = sAIPath->SetPathClosed(yAxis, false);
		
		// Style the axis
		AIPathStyle axisStyle;
		AIBoolean hasAdvFill = false;
		sAIPathStyle->GetPathStyle(yAxis, &axisStyle, &hasAdvFill);
		axisStyle.fillPaint = false;
		axisStyle.strokePaint = true;
		axisStyle.stroke.color.kind = kGrayColor;
		axisStyle.stroke.color.c.g.gray = 0.3 * kAIRealOne;
		axisStyle.stroke.width = 0.5;
		sAIPathStyle->SetPathStyle(yAxis, &axisStyle);
	}
	
	// Add X-axis (horizontal line)
	AIArtHandle xAxis;
	result = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, xAxisGroup, &xAxis);
	if (result == kNoErr) {
		result = sAIPath->SetPathSegmentCount(xAxis, 2);
		AIPathSegment axisSegs[2];
		axisSegs[0].p.h = plotArea.left;
		axisSegs[0].p.v = plotArea.bottom;
		axisSegs[0].in = axisSegs[0].out = axisSegs[0].p;
		axisSegs[0].corner = true;
		
		axisSegs[1].p.h = plotArea.right;
		axisSegs[1].p.v = plotArea.bottom;
		axisSegs[1].in = axisSegs[1].out = axisSegs[1].p;
		axisSegs[1].corner = true;
		
		for (int j = 0; j < 2; j++) {
			result = sAIPath->SetPathSegments(xAxis, j, 1, &axisSegs[j]);
		}
		result = sAIPath->SetPathClosed(xAxis, false);
		
		// Style the axis
		AIPathStyle axisStyle;
		AIBoolean hasAdvFill = false;
		sAIPathStyle->GetPathStyle(xAxis, &axisStyle, &hasAdvFill);
		axisStyle.fillPaint = false;
		axisStyle.strokePaint = true;
		axisStyle.stroke.color.kind = kGrayColor;
		axisStyle.stroke.color.c.g.gray = 0.3 * kAIRealOne;
		axisStyle.stroke.width = 0.5;
		sAIPathStyle->SetPathStyle(xAxis, &axisStyle);
	}
	
	// Add Y-axis labels and tick marks (for 0, 25, 50, 75, 100)
	for (int i = 0; i <= 4; i++) {
		// Y-axis label
		AITextOrientation orient = kHorizontalTextOrientation;
		
		// Get the default font size to adjust vertical position
		AIReal fontSize = 12.0; // Default font size
		ATE::ICharFeatures defaultCharFeatures;
		bool isAssigned = false;
		AIReal actualFontSize = defaultCharFeatures.GetFontSize(&isAssigned);
		if (isAssigned) {
			fontSize = actualFontSize;
		}
		
		AIRealPoint anchor;
		anchor.h = plotArea.left - labelGap - 5;  // Position closer to axis for right alignment
		// Adjust vertical position by moving down by 0.35 * fontSize (half of 0.7 cap height estimate)
		anchor.v = plotArea.bottom + (i * plotHeight / 4) - (fontSize * 0.35);
		
		AIArtHandle yLabel = nullptr;
		result = sAITextFrame->NewPointText(kPlaceInsideOnTop, yLabelsGroup, orient, anchor, &yLabel);
		if (result == kNoErr && yLabel) {
			TextRangeRef range = nullptr;
			result = sAITextFrame->GetATETextRange(yLabel, &range);
			if (result == kNoErr && range) {
				ATE::ITextRange textRange(range);
				
				// Set the text content
				ai::UnicodeString labelText;
				ai::NumberFormat numFormat;
				numFormat.toString(i * 25.0, 0, labelText);
				labelText = labelText + ai::UnicodeString("%");
				textRange.InsertAfter(labelText.as_ASUnicode().c_str());
				
				// Set paragraph alignment to right
				ATE::IParaFeatures paraFeatures;
				paraFeatures.SetJustification(ATE::kRightJustify);
				textRange.SetLocalParaFeatures(paraFeatures);
			}
		}
		
		// Add tick mark
		AIArtHandle tick;
		result = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, yTicksGroup, &tick);
		if (result == kNoErr) {
			result = sAIPath->SetPathSegmentCount(tick, 2);
			AIPathSegment tickSegs[2];
			tickSegs[0].p.h = plotArea.left;
			tickSegs[0].p.v = plotArea.bottom + (i * plotHeight / 4);
			tickSegs[0].in = tickSegs[0].out = tickSegs[0].p;
			tickSegs[0].corner = true;
			
			tickSegs[1].p.h = plotArea.left - 5;  // 5 point tick extending left
			tickSegs[1].p.v = plotArea.bottom + (i * plotHeight / 4);
			tickSegs[1].in = tickSegs[1].out = tickSegs[1].p;
			tickSegs[1].corner = true;
			
			for (int j = 0; j < 2; j++) {
				result = sAIPath->SetPathSegments(tick, j, 1, &tickSegs[j]);
			}
			result = sAIPath->SetPathClosed(tick, false);
			
			// Style the tick mark
			AIPathStyle tickStyle;
			AIBoolean hasAdvFill = false;
			sAIPathStyle->GetPathStyle(tick, &tickStyle, &hasAdvFill);
			tickStyle.fillPaint = false;
			tickStyle.strokePaint = true;
			tickStyle.stroke.color.kind = kGrayColor;
			tickStyle.stroke.color.c.g.gray = 0.3 * kAIRealOne;
			tickStyle.stroke.width = 0.5;
			sAIPathStyle->SetPathStyle(tick, &tickStyle);
		}
	}
	}
	catch (ai::Error& ex) {
		result = ex;
		if (chartArt && *chartArt) {
			sAIArt->DisposeArt(*chartArt);
			*chartArt = nullptr;
		}
	}
	
	return result;
}