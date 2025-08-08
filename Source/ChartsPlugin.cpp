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
#include "ChartsPlugin.h"
#include "ChartItem.h"

ChartsPlugin *gPlugin = NULL;

Plugin *AllocatePlugin(SPPluginRef pluginRef)
{
	return new ChartsPlugin(pluginRef);
}

/*
*/
void FixupReload(Plugin* plugin)
{
	ChartsPlugin::FixupVTable((ChartsPlugin*) plugin);
}

/*
*/
ASErr ChartsPlugin::SetGlobal(Plugin* plugin)
{
	gPlugin = (ChartsPlugin*) plugin;
	return kNoErr;
}

/*
*/
ChartsPlugin::ChartsPlugin(SPPluginRef pluginRef) :
	Plugin(pluginRef), fToolHandle(NULL), fAboutPluginMenu(NULL), 
	fAnnotatorHandle(NULL), fNotifySelectionChanged(NULL),
	fAnnotator(NULL),
    fResourceManagerHandle(NULL),
	fShutdownApplicationNotifier(NULL),
	fChartPluginGroupHandle(NULL)
{	
	strncpy(fPluginName, kChartsPluginName, kMaxStringLength);
}

/*
*/
ASErr ChartsPlugin::StartupPlugin(SPInterfaceMessage* message)
{
	ASErr result = kNoErr;
	try 
	{
		result = Plugin::StartupPlugin(message);
		aisdk::check_ai_error(result);
		// Add About Plugins menu item for this plug-in.
		SDKAboutPluginsHelper aboutPluginsHelper;
		result = aboutPluginsHelper.AddAboutPluginsMenuItem(message, 
					kSDKDefAboutSDKCompanyPluginsGroupName, 
					ai::UnicodeString(kSDKDefAboutSDKCompanyPluginsGroupNameString), 
					"Charts...", 
					&fAboutPluginMenu);
		aisdk::check_ai_error(result);

		// Add tool.
		result = this->AddTool(message);
		aisdk::check_ai_error(result);

		// Add annotator.
		result = this->AddAnnotator(message);
		aisdk::check_ai_error(result);

		// Add notifier.
		result = this->AddNotifier(message);
		aisdk::check_ai_error(result);

		// Register plugin group for custom chart art objects (following LiveDropShadow pattern)
		AIAddPluginGroupData pluginGroupData;
		pluginGroupData.major = 1;
		pluginGroupData.minor = 0;
		char description[] = "Chart Plugin Group";
		pluginGroupData.desc = description;
		char name[] = "ChartPluginGroup";

		result = sAIPluginGroup->AddAIPluginGroup(
			message->d.self,
			name,
			&pluginGroupData,
			kPluginGroupWantsAutoTransformOption,
			&fChartPluginGroupHandle
		);
		aisdk::check_ai_error(result);

		// Set the default name that appears in the Layers panel
		result = sAIPluginGroup->SetAIPluginGroupDefaultName(
			fChartPluginGroupHandle, 
			"Chart"
		);
		aisdk::check_ai_error(result);

	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::PostStartupPlugin()
{
	ASErr result = kNoErr;
	try {
		if (fAnnotator == NULL) {
			fAnnotator = new Charts();
			SDK_ASSERT(fAnnotator);
			// Set the plugin group handle so Charts can create custom art objects
			fAnnotator->SetPluginGroupHandle(fChartPluginGroupHandle);
		}
        result = sAIUser->CreateCursorResourceMgr(fPluginRef,&fResourceManagerHandle);
        aisdk::check_ai_error(result);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::ShutdownPlugin(SPInterfaceMessage* message)
{
	ASErr result = kNoErr;
	try {
		result = Plugin::ShutdownPlugin( message );
		aisdk::check_ai_error(result);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch (...) {
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::Message(char* caller, char* selector, void* message)
{
	ASErr result = kNoErr;
	try {
		result = Plugin::Message(caller, selector, message);
		
		if (result == kUnhandledMsgErr) {
			if (strcmp(caller, kCallerAIAnnotation) == 0) {
				if (strcmp(selector, kSelectorAIDrawAnnotation) == 0) {
					result = this->DrawAnnotation((AIAnnotatorMessage*)message);
					aisdk::check_ai_error(result);
				}
				else if (strcmp(selector, kSelectorAIInvalAnnotation) == 0) {
					result = this->InvalAnnotation((AIAnnotatorMessage*)message);
					aisdk::check_ai_error(result);
				}
			}
			else if (strcmp(caller, kCallerAIPluginGroup) == 0) {
				if (strcmp(selector, kSelectorAIUpdateArt) == 0) {
					result = this->PluginGroupUpdate((AIPluginGroupMessage*)message);
					aisdk::check_ai_error(result);
				}
				else if (strcmp(selector, kSelectorAINotifyEdits) == 0) {
					result = this->PluginGroupNotify((AIPluginGroupMessage*)message);
					aisdk::check_ai_error(result);
				}
			}
		}
		else
			aisdk::check_ai_error(result);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch (...) {
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::GoMenuItem(AIMenuMessage* message)
{
	ASErr result = kNoErr;
	try 
	{
		if (message->menuItem == fAboutPluginMenu) {
			SDKAboutPluginsHelper aboutPluginsHelper;
			aboutPluginsHelper.PopAboutBox(message, "About Charts", kSDKDefAboutSDKCompanyPluginsAlertString);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	
	return result;
}

/*
*/
ASErr ChartsPlugin::UpdateMenuItem(AIMenuMessage* message)
{
	ASErr result = kNoErr;
	try 
	{
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::Notify(AINotifierMessage* message)
{
	ASErr result = kNoErr;
	
	try 
	{
		if (message->notifier == fNotifySelectionChanged) {
			// Get the bounds of the current document view.
			AIRealRect viewBounds = {0, 0, 0, 0};
			result = sAIDocumentView->GetDocumentViewBounds(NULL, &viewBounds);
			aisdk::check_ai_error(result);
			// Invalidate the entire document view bounds.
			result = fAnnotator->InvalidateRect(viewBounds);
			aisdk::check_ai_error(result);
		}
		if (message->notifier == fShutdownApplicationNotifier)
		{
			if(fResourceManagerHandle != NULL)
			{
				sAIUser->DisposeCursorResourceMgr(fResourceManagerHandle);
				fResourceManagerHandle = NULL;
			}
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}

	return result;
}

/*
*/
ASErr ChartsPlugin::TrackToolCursor(AIToolMessage* message)
{
	ASErr result = kNoErr;
	try {
		if (this->fAnnotator) {
			// Track cursor for rectangle drawing and smart guides
			result = this->fAnnotator->TrackCursor(message);
			aisdk::check_ai_error(result);
			
			// Set crosshair cursor
			if(sAIUser != NULL)
				result = sAIUser->SetCursor(kAICrossCursorID, fResourceManagerHandle);
            aisdk::check_ai_error(result);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::ToolMouseDown(AIToolMessage* message)
{
	ASErr result = kNoErr;
	try {
		if (this->fAnnotator) {
			result = this->fAnnotator->MouseDown(message);
			aisdk::check_ai_error(result);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::ToolMouseDrag(AIToolMessage* message)
{
	ASErr result = kNoErr;
	try {
		if (this->fAnnotator) {
			result = this->fAnnotator->MouseDrag(message);
			aisdk::check_ai_error(result);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::ToolMouseUp(AIToolMessage* message)
{
	ASErr result = kNoErr;
	try {
		if (this->fAnnotator) {
			result = this->fAnnotator->MouseUp(message);
			aisdk::check_ai_error(result);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::SelectTool(AIToolMessage* message)
{
	ASErr result = kNoErr;
	try {
		result = sAIAnnotator->SetAnnotatorActive(fAnnotatorHandle, true);
		aisdk::check_ai_error(result);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::DeselectTool(AIToolMessage* message)
{
	ASErr result = kNoErr;
	try {
		result = sAIAnnotator->SetAnnotatorActive(fAnnotatorHandle, false);
		aisdk::check_ai_error(result);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::AddTool(SPInterfaceMessage *message)
{
	ASErr result = kNoErr;
	try {
		ASErr			error = kNoErr;
		AIAddToolData	data;
        data.title = ai::UnicodeString::FromRoman("SDK Charts Tool");
		data.tooltip = ai::UnicodeString::FromRoman("SDK Charts Tool");
		data.sameGroupAs = kNoTool;
		data.sameToolsetAs = kNoTool;
		data.normalIconResID = kChartsToolIconResourceID;
		data.darkIconResID = kChartsToolDarkIconResourceID;
		data.iconType = ai::IconType::kSVG;

		ai::int32 options = kToolWantsToTrackCursorOption;

		error = sAITool->GetToolNumberFromName(kChartsTool, &data.sameGroupAs); // the Crop Area tool is now the first in its group
		if (error) return error;

		error = sAITool->GetToolNumberFromName(kChartsTool, &data.sameToolsetAs); // and at the top of its tool stack
		if (error) return error;

		if (!result) {
			result = sAITool->AddTool(message->d.self, kChartsTool, data, options, &fToolHandle);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::AddAnnotator(SPInterfaceMessage *message)
{
	ASErr result = kNoErr;
	try {
		result = sAIAnnotator->AddAnnotator(message->d.self, "SDK Charts", &fAnnotatorHandle);
		aisdk::check_ai_error(result);

		result = sAIAnnotator->SetAnnotatorActive(fAnnotatorHandle, false);
		aisdk::check_ai_error(result);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::AddNotifier(SPInterfaceMessage *message)
{
	ASErr result = kNoErr;
	try {
		result = sAINotifier->AddNotifier(fPluginRef, "ChartsPlugin", kAIArtSelectionChangedNotifier, &fNotifySelectionChanged);
		aisdk::check_ai_error(result);
		result = sAINotifier->AddNotifier(fPluginRef, "ChartsPlugin", kAIApplicationShutdownNotifier, &fShutdownApplicationNotifier);
		aisdk::check_ai_error(result);

	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::DrawAnnotation(AIAnnotatorMessage* message)
{
	ASErr result = kNoErr;
	try {
		if (this->fAnnotator) {
			// Only draw the rectangle preview annotation
			result = this->fAnnotator->DrawRectanglePreview(message);
			aisdk::check_ai_error(result);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::InvalAnnotation(AIAnnotatorMessage* message)
{
	ASErr result = kNoErr;
	try {
		result = fAnnotator->InvalidateRect(*message->invalidationRects);
		aisdk::check_ai_error(result);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch(...)
	{
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::PluginGroupUpdate(AIPluginGroupMessage* message)
{
	ASErr result = kNoErr;
	try {
		AIArtHandle pluginArt = message->art;
		
		// Get edit and result groups
		AIArtHandle editArt = NULL;
		AIArtHandle resultArt = NULL;
		result = sAIPluginGroup->GetPluginArtEditArt(pluginArt, &editArt);
		aisdk::check_ai_error(result);
		result = sAIPluginGroup->GetPluginArtResultArt(pluginArt, &resultArt);
		aisdk::check_ai_error(result);
		
		// Clear all children from the result group
		AIArtHandle child = nullptr;
		result = sAIArt->GetArtFirstChild(resultArt, &child);
		aisdk::check_ai_error(result);

		while (child != nullptr) {
			AIArtHandle nextChild = nullptr;
			result = sAIArt->GetArtSibling(child, &nextChild);
			aisdk::check_ai_error(result);
			
			result = sAIArt->DisposeArt(child);
			aisdk::check_ai_error(result);
			
			child = nextChild;
		}
		
		// Load custom chart data
		ChartItem chartData;
		size_t dataSize = sizeof(ChartItem);
		result = sAIPluginGroup->GetPluginArtDataRange(pluginArt, &chartData, 0, dataSize);
		if (result == kNoErr) {
			// Set the result group as the parent for chart rendering
			chartData.SetChartGroup(resultArt);
			
			// Recreate the chart content in the result group
			result = chartData.CreateChartArt();
			aisdk::check_ai_error(result);
		}
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch (...) {
		result = kCantHappenErr;
	}
	return result;
}

/*
*/
ASErr ChartsPlugin::PluginGroupNotify(AIPluginGroupMessage* message)
{
	ASErr result = kNoErr;
	try {
		// Handle notification of edits to the plugin group
		// For now, we'll just update the art
		result = PluginGroupUpdate(message);
	}
	catch (ai::Error& ex) {
		result = ex;
	}
	catch (...) {
		result = kCantHappenErr;
	}
	return result;
}

// end ChartsPlugin.cpp
