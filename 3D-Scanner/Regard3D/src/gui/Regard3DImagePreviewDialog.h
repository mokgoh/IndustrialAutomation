/**
 * Copyright (C) 2015 Roman Hiestand
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef REGARD3DIMAGEPREVIEWDIALOG_H
#define REGARD3DIMAGEPREVIEWDIALOG_H

class PreviewGeneratorThread;

#include "PreviewGeneratorThread.h"
#include "R3DProject.h"

// Generated by wxFormBuilder
#include "Regard3DMainFrameBase.h"

class Regard3DImagePreviewDialog: public Regard3DImagePreviewDialogBase
{
public:
	Regard3DImagePreviewDialog(wxWindow *pParent);
	virtual ~Regard3DImagePreviewDialog();

	void setImage(const wxImage &img);
	void setPreviewInfo(const PreviewInfo &previewInfo);
	void setPreviewGeneratorThread(PreviewGeneratorThread *pPreviewGeneratorThread);
	void setComputeMatches(R3DProject *pProject, R3DProject::ComputeMatches *pComputeMatches);

protected:
	virtual void OnClose( wxCloseEvent& event );
	virtual void OnInitDialog( wxInitDialogEvent& event );
	virtual void OnKeypointTypeChoice( wxCommandEvent& event );
	virtual void OnShowSingleKeypointsCheckBox( wxCommandEvent& event );
	virtual void OnMatchesChoice( wxCommandEvent& event );
	virtual void OnEnableTrackFilter( wxCommandEvent& event );
	virtual void OnZoomInButton( wxCommandEvent& event );
	virtual void OnZoomOutButton( wxCommandEvent& event );
	virtual void OnCloseButton( wxCommandEvent& event );

	virtual void OnMouseWheel(wxMouseEvent& event);
	virtual void OnTimer( wxTimerEvent &event );

	void updateZoomFactor(double zoomFactor);

private:
	static const double zoomChange_;

	wxTimer aTimer_;
	PreviewInfo previewInfo_;
	PreviewGeneratorThread *pPreviewGeneratorThread_;
	R3DProject *pProject_;
	R3DProject::ComputeMatches *pComputeMatches_;
	R3DProject::PictureSet *pPictureSet_;
	R3DProjectPaths paths_;

	DECLARE_EVENT_TABLE()
};

#endif
