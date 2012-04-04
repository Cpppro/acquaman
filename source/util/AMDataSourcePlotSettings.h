#ifndef AMDATASOURCEPLOTSETTINGS_H
#define AMDATASOURCEPLOTSETTINGS_H

#include <QPen>
#include <QColor>

#include "MPlot/MPlotColorMap.h"

/// This class holds visualization information about AMDataSources; all the plot settings that are associated with a particular plot/layout, rather than with the AMDataSource itself.
class AMDataSourcePlotSettings {
public:
	/// Default Constructor
	AMDataSourcePlotSettings(double Priority = 1, const QPen& LinePen = QPen(nextColor()))
		: priority(Priority),
		  // visible(Visible),
		  linePen(LinePen),
		  colorMap(MPlotColorMap::Jet)
	{

		areaFilled = false;

		colorMap.setContrast(2.1);
		colorMap.setBrightness(0.08);
		colorMap.setGamma(0.8);
	}



	/// Priority level for this data source (used for ordering... lower numbers appear first.)
	double priority;
	/// Whether this data source is shown/enabled in non-exclusive views. This option is available to users; they can toggle it on or off.
	// Now stored in AMDataSource::visibleInPlots(). bool visible;

	// 1D plot settings:
	/// Pen used for this data source (dots, dashes, etc.), as well as width and color
	QPen linePen;
	/// True if the area below the plot should be filled \note These don't work yet, since MPlot doesn't yet support filled plots
	bool areaFilled;
	/// The brush of the fill, if used (ie: areaFilled is true) \note These don't work yet, since MPlot doesn't yet support filled plots
	QBrush fillBrush;

	// 2D plot settings:

	/// Resultant colormap used for multi-dimensional data
	MPlotColorMap colorMap;


	/// Globally-accessible function to get the "next" data source color to use.
	static QColor nextColor() {
		static int i = 0;

		switch(i++ % 11) {
		case 0: return QColor(255, 0, 128);
		case 1: return QColor(0, 128, 255);
		case 2: return QColor(128, 255, 0);
		case 3: return QColor(255, 128, 0);
		case 4: return QColor(128, 0, 255);
		case 5: return QColor(0, 0, 255);
		case 6: return QColor(0, 128, 0);
			// Yellow is hard to see: case 7: return QColor(255, 255, 0);
		case 7: return QColor(255, 0, 0);
		case 8: return QColor(0, 64, 128);
		case 9: return QColor(128, 64, 0);
		case 10: default: return QColor(128, 0, 64);
		}
	}

};

#endif // AMDATASOURCEPLOTSETTINGS_H