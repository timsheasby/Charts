//========================================================================================
//  
//  ChartItem.h
//
//  Copyright 2024 Adobe Systems Incorporated. All rights reserved.
//  
//  NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance 
//  with the terms of the Adobe license agreement accompanying it.  If you have received
//  this file from a source other than Adobe, then your use, modification, or 
//  distribution of it requires the prior written permission of Adobe.
//  
//========================================================================================

#ifndef __ChartItem_h__
#define __ChartItem_h__

#include "IllustratorSDK.h"
#include <vector>
#include <string>

// Dictionary keys for storing chart data
#define kChartTypeDictKey			"ChartType"
#define kChartTitleDictKey			"ChartTitle"
#define kChartXAxisLabelDictKey		"ChartXAxisLabel"
#define kChartYAxisLabelDictKey		"ChartYAxisLabel"
#define kChartDataSeriesDictKey		"ChartDataSeries"
#define kChartShowLegendDictKey		"ChartShowLegend"
#define kChartShowGridDictKey		"ChartShowGrid"
#define kChartShowDataLabelsDictKey	"ChartShowDataLabels"
#define kChartMarginDictKey			"ChartMargin"
#define kChartIDDictKey				"ChartID"
#define kChartVersionDictKey		"ChartVersion"

// Chart art type identifier
#define kChartArtType				"com.adobe.illustrator.charts.chartObject"

// Chart type enumeration
enum ChartType {
	kChartTypeBar = 0,
	kChartTypeLine,
	kChartTypePie,
	kChartTypeArea,
	kChartTypeScatter,
	kChartTypeColumn,
	kChartTypeDonut,
	kChartTypeRadar,
	kChartTypeUnknown
};

// Data point structure
struct ChartDataPoint {
	AIReal value;
	ai::UnicodeString label;
	AIRGBColor color;
	
	ChartDataPoint() : value(0.0) {
		color.red = 0;
		color.green = 0;
		color.blue = 0;
	}
	
	ChartDataPoint(AIReal v, const ai::UnicodeString& l) : value(v), label(l) {
		// Default color
		color.red = 30000;
		color.green = 30000;
		color.blue = 30000;
	}
};

// Data series for multi-series charts
struct ChartDataSeries {
	ai::UnicodeString name;
	std::vector<ChartDataPoint> dataPoints;
	AIRGBColor seriesColor;
	
	ChartDataSeries() {
		seriesColor.red = 30000;
		seriesColor.green = 30000;
		seriesColor.blue = 30000;
	}
};

// Chart item class
class ChartItem {
private:
	// Chart bounds (the rectangle that defines the chart area)
	AIRealRect fBounds;
	
	// Chart type
	ChartType fChartType;
	
	// Chart data
	std::vector<ChartDataSeries> fDataSeries;
	
	// Chart properties
	ai::UnicodeString fTitle;
	ai::UnicodeString fXAxisLabel;
	ai::UnicodeString fYAxisLabel;
	
	// Visual properties
	AIBoolean fShowLegend;
	AIBoolean fShowGrid;
	AIBoolean fShowDataLabels;
	AIReal fMargin;  // Margin inside the bounds for chart content
	
	// Reference to the Illustrator art group containing the chart
	AIArtHandle fChartGroup;
	
	// Unique identifier for this chart
	ai::int32 fChartID;
	static ai::int32 sNextChartID;

public:
	// Constructor
	ChartItem();
	ChartItem(const AIRealRect& bounds, ChartType type);
	
	// Destructor
	~ChartItem();
	
	// Bounds management
	void SetBounds(const AIRealRect& bounds) { fBounds = bounds; }
	const AIRealRect& GetBounds() const { return fBounds; }
	
	// Chart type
	void SetChartType(ChartType type) { fChartType = type; }
	ChartType GetChartType() const { return fChartType; }
	
	// Data management
	void AddDataSeries(const ChartDataSeries& series);
	void ClearData();
	size_t GetSeriesCount() const { return fDataSeries.size(); }
	ChartDataSeries* GetSeries(size_t index);
	const ChartDataSeries* GetSeries(size_t index) const;
	
	// Add single data point (for simple single-series charts)
	void AddDataPoint(AIReal value, const ai::UnicodeString& label);
	void AddDataPoint(const ChartDataPoint& point);
	
	// Properties
	void SetTitle(const ai::UnicodeString& title) { fTitle = title; }
	const ai::UnicodeString& GetTitle() const { return fTitle; }
	
	void SetXAxisLabel(const ai::UnicodeString& label) { fXAxisLabel = label; }
	const ai::UnicodeString& GetXAxisLabel() const { return fXAxisLabel; }
	
	void SetYAxisLabel(const ai::UnicodeString& label) { fYAxisLabel = label; }
	const ai::UnicodeString& GetYAxisLabel() const { return fYAxisLabel; }
	
	void SetShowLegend(AIBoolean show) { fShowLegend = show; }
	AIBoolean GetShowLegend() const { return fShowLegend; }
	
	void SetShowGrid(AIBoolean show) { fShowGrid = show; }
	AIBoolean GetShowGrid() const { return fShowGrid; }
	
	void SetShowDataLabels(AIBoolean show) { fShowDataLabels = show; }
	AIBoolean GetShowDataLabels() const { return fShowDataLabels; }
	
	void SetMargin(AIReal margin) { fMargin = margin; }
	AIReal GetMargin() const { return fMargin; }
	
	// Art handle
	void SetChartGroup(AIArtHandle group) { fChartGroup = group; }
	AIArtHandle GetChartGroup() const { return fChartGroup; }
	
	// Chart ID
	ai::int32 GetChartID() const { return fChartID; }
	
	// Chart rendering
	ASErr CreateChartArt();  // Creates the Illustrator artwork for the chart
	ASErr UpdateChartArt();  // Updates existing chart artwork
	ASErr DeleteChartArt();  // Removes the chart artwork

	// Plugin art creation
	static ASErr CreatePluginArt(const AIRealRect& bounds, ChartType type, AIPluginGroupHandle pluginGroupHandle, AIArtHandle* chartArt);
	
	// Render chart content in existing group (for plugin art)
	ASErr RenderChartContent();
	
	// Data validation
	AIBoolean ValidateData() const;
	
	// Get chart type as string
	ai::UnicodeString GetChartTypeString() const;
	
	// Dictionary operations for custom art object
	ASErr WriteToDictionary(AIDictionaryRef dict) const;  // Write chart data to dictionary
	ASErr ReadFromDictionary(AIDictionaryRef dict);  // Read chart data from dictionary
	
	// Static method to check if art is a chart object
	static AIBoolean IsChartArt(AIArtHandle art);
	
	// Static method to create ChartItem from existing art
	static ChartItem* CreateFromArt(AIArtHandle art);
	
private:
	// Helper methods for rendering different chart types
	ASErr RenderBarChart();
	ASErr RenderLineChart();
	ASErr RenderPieChart();
	ASErr RenderAreaChart();
	ASErr RenderScatterChart();
	ASErr RenderColumnChart();
	ASErr RenderDonutChart();
	ASErr RenderRadarChart();
	
	// Helper for creating chart background
	ASErr CreateChartBackground();
	
	// Helper for creating axes
	ASErr CreateAxes();
	
	// Helper for creating legend
	ASErr CreateLegend();
	
	// Helper for calculating data ranges
	void CalculateDataRange(AIReal& minValue, AIReal& maxValue) const;
};

#endif // __ChartItem_h__