/**************************************************************************
 *  copyright            : (C) 2004-2006 by Petr Schwarz & Pavel Matejka  *
 *                                        UPGM,FIT,VUT,Brno               *
 *  email                : {schwarzp,matejkap}@fit.vutbr.cz               *
 **************************************************************************
 *                                                                        *
 *  This program is free software; you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation; either version 2 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 **************************************************************************/

#ifndef _THRESHOLDS_H
#define _THRESHOLDS_H

#include <string>
#include <map>

class Thresholds
{
	protected:
		std::map<std::string, float> thrs;
		float defaultThr;

	public:
		Thresholds();
		bool LoadThresholds(char *file);
		void SetDefaultThr(float v) {defaultThr = v;};
		float GetThreshold(char *word);
		void Clear() {thrs.clear();}
};

#endif
