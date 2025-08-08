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
		
		// Create the visual chart content directly in the group
		// Background rectangle
		AIArtHandle backgroundRect;
		result = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, resultArt, &backgroundRect);
		aisdk::check_ai_error(result);
		
		// Set up the background rectangle path
		result = sAIPath->SetPathSegmentCount(backgroundRect, 4);
		aisdk::check_ai_error(result);
		
		AIPathSegment segments[4];
		// Top-left, Top-right, Bottom-right, Bottom-left
		segments[0].p.h = bounds.left; segments[0].p.v = bounds.top;
		segments[1].p.h = bounds.right; segments[1].p.v = bounds.top;
		segments[2].p.h = bounds.right; segments[2].p.v = bounds.bottom;
		segments[3].p.h = bounds.left; segments[3].p.v = bounds.bottom;
		
		for (int i = 0; i < 4; i++) {
			segments[i].in = segments[i].out = segments[i].p;
			segments[i].corner = true;
			result = sAIPath->SetPathSegments(backgroundRect, i, 1, &segments[i]);
			aisdk::check_ai_error(result);
		}
		
		result = sAIPath->SetPathClosed(backgroundRect, true);
		aisdk::check_ai_error(result);
		
		// Set background style - white fill with gray stroke
		AIPathStyle style;
		AIBoolean hasAdvFill = false;
		result = sAIPathStyle->GetPathStyle(backgroundRect, &style, &hasAdvFill);
		aisdk::check_ai_error(result);
		
		style.fillPaint = true;
		style.fill.color.kind = kGrayColor;
		style.fill.color.c.g.gray = 0;  // White
		
		style.strokePaint = true;
		style.stroke.color.kind = kGrayColor;
		style.stroke.color.c.g.gray = 0.7 * kAIRealOne;  // Light gray
		style.stroke.width = 1.0;
		
		result = sAIPathStyle->SetPathStyle(backgroundRect, &style);
		aisdk::check_ai_error(result);
		
		// Create a simple bar for visual feedback
		AIReal margin = 20.0;
		AIRealRect barBounds;
		barBounds.left = bounds.left + margin;
		barBounds.right = bounds.right - margin;
		barBounds.top = bounds.top + margin;
		barBounds.bottom = bounds.bottom - margin;
		
		AIArtHandle barRect;
		result = sAIArt->NewArt(kPathArt, kPlaceInsideOnTop, resultArt, &barRect);
		aisdk::check_ai_error(result);
		
		result = sAIPath->SetPathSegmentCount(barRect, 4);
		aisdk::check_ai_error(result);
		
		// Bar rectangle segments
		segments[0].p.h = barBounds.left; segments[0].p.v = barBounds.top;
		segments[1].p.h = barBounds.right; segments[1].p.v = barBounds.top;
		segments[2].p.h = barBounds.right; segments[2].p.v = barBounds.bottom;
		segments[3].p.h = barBounds.left; segments[3].p.v = barBounds.bottom;
		
		for (int i = 0; i < 4; i++) {
			result = sAIPath->SetPathSegments(barRect, i, 1, &segments[i]);
			aisdk::check_ai_error(result);
		}
		
		result = sAIPath->SetPathClosed(barRect, true);
		aisdk::check_ai_error(result);
		
		// Set bar style - blue fill
		result = sAIPathStyle->GetPathStyle(barRect, &style, &hasAdvFill);
		aisdk::check_ai_error(result);
		
		style.fillPaint = true;
		style.fill.color.kind = kThreeColor;
		style.fill.color.c.rgb.red = 0;
		style.fill.color.c.rgb.green = 30000;
		style.fill.color.c.rgb.blue = 65000;
		style.strokePaint = false;
		
		result = sAIPathStyle->SetPathStyle(barRect, &style);
		aisdk::check_ai_error(result);
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