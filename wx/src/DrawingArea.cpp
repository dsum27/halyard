// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; -*-

#include "TamaleHeaders.h"
#include "DrawingArea.h"
#include "DrawingAreaOpt.h"
#include "Stage.h"
#include "CommonWxConv.h"

DrawingArea::DrawingArea(Stage *inStage, int inWidth, int inHeight,
						 bool inHasAlpha)
	: mStage(inStage), mBounds(wxPoint(0, 0), wxSize(inWidth, inHeight))
{
	InitializePixmap(inHasAlpha);
}

DrawingArea::DrawingArea(Stage *inStage, const wxRect &inBounds,
						 bool inHasAlpha)
	: mStage(inStage), mBounds(inBounds)
{
	InitializePixmap(inHasAlpha);
}

DrawingArea::~DrawingArea() {
    // We're going away, so force recompositing later.
    InvalidateDrawingArea();
}

void DrawingArea::InitializePixmap(bool inHasAlpha) {
	mPixmap.Create(mBounds.GetWidth(), mBounds.GetHeight(),
				   inHasAlpha ? 32 : 24);
	if (inHasAlpha)
		mPixmap.UseAlpha();
	Clear();
}

void DrawingArea::InvalidateRect(const wxRect &inRect, int inInflate) {
	wxRect r(inRect);
	r.Inflate(inInflate);
	r.Offset(mBounds.GetPosition());
	mStage->InvalidateRect(r);
}

void DrawingArea::InvalidateDrawingArea() {
    InvalidateRect(wxRect(0, 0, mBounds.GetWidth(), mBounds.GetHeight()));
}

void DrawingArea::MoveTo(const wxPoint &inPoint) {
    InvalidateDrawingArea();
    mBounds = wxRect(inPoint, mBounds.GetSize());
    InvalidateDrawingArea();
}

void DrawingArea::Clear() {
	if (mPixmap.HasAlpha()) {
		Clear(GraphicsTools::Color(0x00, 0x00, 0x00, 0x00));
	} else {
		Clear(GraphicsTools::Color(0x00, 0x00, 0x00));
	}
}

void DrawingArea::Clear(const GraphicsTools::Color &inColor) {
	if (mPixmap.HasAlpha()) {
		wxAlphaPixelData data(mPixmap);
		ClearOpt(data, inColor);
	} else if (inColor.IsCompletelyOpaque()) {
		wxMemoryDC dc;
		dc.SelectObject(GetPixmap());
		wxBrush brush(wxColor(inColor.red, inColor.green, inColor.blue),
					  wxSOLID);
		dc.SetBackground(brush);
		dc.Clear();
	} else {
		THROW("Cannot clear opaque overlay with transparent color.");
	}
    InvalidateDrawingArea();
}

void DrawingArea::DrawLine(const wxPoint &inFrom, const wxPoint &inTo,
						   const wxColour &inColor, int inWidth)
{
	wxMemoryDC dc;
	dc.SelectObject(GetPixmap());
	wxPen pen(inColor, inWidth, wxSOLID);
	dc.SetPen(pen);
	dc.DrawLine(inFrom.x, inFrom.y, inTo.x, inTo.y);

	// This is slightly annoying.
	InvalidateRect(wxRect(wxPoint(std::min(inFrom.x, inTo.x),
								  std::min(inFrom.y, inTo.y)),
						  wxPoint(std::max(inFrom.x, inTo.x),
								  std::max(inFrom.y, inTo.y))),
				   inWidth);
}

void DrawingArea::FillBox(const wxRect &inBounds,
						  const GraphicsTools::Color &inColor)
{
	if (mPixmap.HasAlpha()) {
		wxAlphaPixelData data(mPixmap);
		FillBoxOpt(data, inBounds, inColor);
	} else if (inColor.IsCompletelyOpaque()) {
		wxColor color = GraphicsToolsToWxColor(inColor);
		wxMemoryDC dc;
		dc.SelectObject(GetPixmap());
		wxBrush brush(color, wxSOLID);
		dc.SetBrush(brush);
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(inBounds.x, inBounds.y,
						 inBounds.width, inBounds.height);
	} else {
		wxNativePixelData data(mPixmap);
		FillBoxOpt(data, inBounds, inColor);
	}
	InvalidateRect(inBounds);
}

void DrawingArea::OutlineBox(const wxRect &inBounds, const wxColour &inColor,
							 int inWidth)
{
	wxMemoryDC dc;
	dc.SelectObject(GetPixmap());
	wxPen pen(inColor, inWidth, wxSOLID);
	dc.SetPen(pen);
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawRectangle(inBounds.x, inBounds.y,
					 inBounds.width, inBounds.height);
	InvalidateRect(inBounds, inWidth);
}

void DrawingArea::DrawPixMap(GraphicsTools::Point inPoint,
							 GraphicsTools::PixMap &inPixMap)
{
	if (mPixmap.HasAlpha()) {
		wxAlphaPixelData data(mPixmap);
		DrawPixMapOpt(data, inPoint, inPixMap);
	} else {
		wxNativePixelData data(mPixmap);
		DrawPixMapOpt(data, inPoint, inPixMap);		
	}
	InvalidateRect(wxRect(inPoint.x, inPoint.y,
						  inPixMap.width, inPixMap.height));
}

void DrawingArea::DrawBitmap(const wxBitmap &inBitmap,
							 wxCoord inX, wxCoord inY,
							 bool inTransparent)
{
	wxMemoryDC dc;
	dc.SelectObject(GetPixmap());
	dc.DrawBitmap(inBitmap, inX, inY, inTransparent);
    InvalidateRect(wxRect(inX, inY,
                          inBitmap.GetWidth(),
                          inBitmap.GetHeight()));
}

void DrawingArea::DrawDCContents(wxDC &inDC)
{
	wxMemoryDC dc;
	dc.SelectObject(GetPixmap());
	if (!dc.Blit(0, 0, GetPixmap().GetWidth(), GetPixmap().GetHeight(),
				 &inDC, 0, 0))
		Clear(GraphicsTools::Color(0, 0, 0));
	InvalidateRect(wxRect(0, 0,
						  GetPixmap().GetWidth(), GetPixmap().GetHeight()));
}

GraphicsTools::Color DrawingArea::GetPixel(wxCoord inX, wxCoord inY) {
	wxRect local_bounds(0, 0, GetBounds().width, GetBounds().height);
	if (!local_bounds.Inside(wxPoint(inX, inY)))
		THROW("Can't get color of point outside of current drawing area");

	GraphicsTools::Color result;
	if (!HasAlpha()) {
		wxMemoryDC dc;
		dc.SelectObject(GetPixmap());
		wxColour c;
		dc.GetPixel(inX, inY, &c);
		result.red   = c.Red();
		result.green = c.Green();
		result.blue  = c.Blue();
		result.alpha = 255;
	} else {
		wxAlphaPixelData data(GetPixmap());
		wxAlphaPixelData::Iterator i(data);
		i.Offset(data, inX, inY);
		// Attempt to reverse pre-multiplications.
		result.red   = i.Alpha() ? (i.Red()   * 255) / i.Alpha() : 0;
		result.green = i.Alpha() ? (i.Green() * 255) / i.Alpha() : 0;
		result.blue  = i.Alpha() ? (i.Blue()  * 255) / i.Alpha() : 0;
		result.alpha = i.Alpha();
	}
	return result;
}

void DrawingArea::CompositeInto(wxDC &inDC, const wxRect &inClipRect) {
	if (inClipRect.Intersects(mBounds)) {
		// Figure out how much of inClipRect actually applies to us.
		wxRect clip(inClipRect);
		clip.Intersect(mBounds);
		
		// Log this operation so we have some hope of actually being
		// able to debug off-screen compositing.
		wxLogTrace(TRACE_STAGE_DRAWING,
				   "Dirty: %d %d %d %d Bounds: %d %d %d %d Clip: %d %d %d %d",
				   inClipRect.GetLeft(), inClipRect.GetTop(),
				   inClipRect.GetRight(), inClipRect.GetBottom(),
				   mBounds.GetLeft(), mBounds.GetTop(),
				   mBounds.GetRight(), mBounds.GetBottom(),
				   clip.GetLeft(), clip.GetTop(),
				   clip.GetRight(), clip.GetBottom());

		// Do the compositing.
		wxMemoryDC dc;
		dc.SelectObject(GetPixmap());
		wxPoint src_loc(clip.x - mBounds.x, clip.y - mBounds.y);
		if (GetPixmap().HasAlpha()) {
			if (!inDC.AlphaBlend(clip.x, clip.y, clip.width, clip.height,
								 &dc, src_loc.x, src_loc.y))
				gLog.Error("Cannot alpha-composite offscreen layers.");
		} else {
			if (!inDC.Blit(clip.x, clip.y, clip.width, clip.height,
						   &dc, src_loc.x, src_loc.y))
				gLog.Error("Cannot composite offscreen layers.");
		}
	}
}