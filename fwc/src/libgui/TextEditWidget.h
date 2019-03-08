/* 

                          Firewall Builder

                 Copyright (C) 2009 NetCitadel, LLC

  Author:  Vadim Kurland     vadim@fwbuilder.org

  $Id$

  This program is free software which we release under the GNU General Public
  License. You may redistribute and/or modify this program under the terms
  of that license as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  To get a copy of the GNU General Public License, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef __TEXTEDITWIDGET_H__
#define __TEXTEDITWIDGET_H__

#include <QTextEdit>

class QFocusEvent;
class QKeyEvent;

class TextEditWidget : public QTextEdit
{
    Q_OBJECT;

    bool modified;
    bool showingDefault;
    bool hasFocus;
    QString defaultText;

signals:
    void textChanged();
    
public slots:
    void dirty(bool);
    
public:
    TextEditWidget(QWidget *parent);
    virtual void focusInEvent(QFocusEvent *event);
    virtual void focusOutEvent(QFocusEvent* event);

    void setTextDefault(const QString &text, const QString &theDefault);
    QString getText();
};

#endif
