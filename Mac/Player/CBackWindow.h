/////////////////////////////////////////////////////////////////////////////////////////
//
//		(c) Copyright 1999, Trustees of Dartmouth College. All rights reserved
//			Interactive Media Lab, Dartmouth Medical School
//
//			$Autho$
//			$Date$
//			$Revision$
//
//
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
//
//	CBackWindow.h
//
//

class CBackWindow : public LWindow
{
	public:
		enum { class_ID = FOUR_CHAR_CODE('PlBk') };
		
					CBackWindow();
					CBackWindow(LStream *inStream);
	static CBackWindow		*CreateWindow(ResIDT	inWindowID, LCommander*	inSuperCommander)	;
				
	void 			DrawSelf();
	void			FadeAfterDraw(void) { mFadeAfterDraw = true; }	
	
	protected:
		bool		mFadeAfterDraw;
		
};
	

/* 
$Log$
Revision 1.1  1999/10/21 17:19:17  chuck
Initial revisioni

*/