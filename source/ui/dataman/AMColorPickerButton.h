/*
Copyright 2010, 2011 Mark Boots, David Chevrier, and Darren Hunter.

This file is part of the Acquaman Data Acquisition and Management framework ("Acquaman").

Acquaman is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Acquaman is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Acquaman.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef AMCOLORPICKERBUTTON_H
#define AMCOLORPICKERBUTTON_H

#include <QToolButton>

class QColorDialog;

class AMColorPickerButton : public QToolButton
{
	Q_OBJECT
public:
	AMColorPickerButton(const QColor& initialColor, QWidget *parent = 0);
	explicit AMColorPickerButton(QWidget* parent = 0);

	/// Retrieve the color currently displayed/selected
	QColor color() const { return currentColor_; }

signals:
	/// Emitted when the current color changes
	void colorChanged(const QColor& newColor);

public slots:

	/// Call to set the color currently displayed/selected
	void setColor(const QColor& newColor);

	/// Called either on click, or programmatically, to show the color browser and choose a new color.
	void activateColorBrowser();

protected slots:
	/// Called when the color dialog is finished (user has selected a color)
	void onColorDialogAccepted();
	/// Called when the color dialog is rejected (user has cancelled)
	void onColorDialogRejected();

protected:
	QColorDialog* colorDialog_;
	QColor currentColor_;

};

#endif // AMCOLORPICKERBUTTON_H