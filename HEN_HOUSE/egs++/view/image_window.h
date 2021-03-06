/*
###############################################################################
#
#  EGSnrc egs++ geometry viewer image window headers
#  Copyright (C) 2015 National Research Council Canada
#
#  This file is part of EGSnrc.
#
#  EGSnrc is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Affero General Public License as published by the
#  Free Software Foundation, either version 3 of the License, or (at your
#  option) any later version.
#
#  EGSnrc is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
#  FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for
#  more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with EGSnrc. If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################
#
#  Author:          Iwan Kawrakow, 2005
#
#  Contributors:    Frederic Tessier
#
###############################################################################
*/


#ifndef IMAGE_WINDOW_
#define IMAGE_WINDOW_

#include "egs_libconfig.h"
#include "egs_functions.h"

#include <qdialog.h>
#include <qtimer.h>

// #include "egs_functions.h"
// #define VIEW_DEBUG

//class ImageWindow : public QDialog {
class ImageWindow : public QWidget {

    Q_OBJECT


public:

    //ImageWindow(QWidget *parent = 0, const char *name = 0, bool modal = FALSE,
    //      WFlags f = 0 ) : QDialog(parent,name,modal,f), resizing(false) { };
    ImageWindow(QWidget *parent=0, const char *name=0, WFlags f=0 ) : QWidget(parent,name,f), resizing(false) {
            navigationTimer = new QTimer(this);
            connect (navigationTimer, SIGNAL(timeout()), parent, SLOT(endTransformation()));
            navigating=false;
            setMouseTracking(true);}
    ~ImageWindow() {};
    void polish() {
        //QDialog::polish();
        QWidget::polish();
        QWidget *topl = topLevelWidget();
        //egsWarning("In polish: position: %d %d\n",pos().x(),pos().y());
        //if( !topl ) egsWarning("Null top level widget!\n");
        QWidget *parent = parentWidget();
        if( !parent ) parent = topl;
        //egsWarning("parent: %s\n",parent->name());
        if( parent ) {
            QPoint point = parent->mapToGlobal(QPoint(0,0));
            //egsWarning("parent: %d %d\n",point.x(),point.y());
            //QRect my_frame = frameGeometry();
            //egsWarning("my geometry: %d %d %d %d\n",my_frame.left(),
            //     my_frame.right(),my_frame.top(),my_frame.bottom());
            int gview_x = point.x();
            int gview_y = point.y() + parent->height();
            //egsWarning("moving to %d %d\n",gview_x,gview_y);
            move(gview_x,gview_y);
            //my_frame = frameGeometry();
            //egsWarning("my geometry: %d %d %d %d\n",my_frame.left(),
            //     my_frame.right(),my_frame.top(),my_frame.bottom());
        }
    };

    int xMouse, yMouse;

protected:

    void resizeEvent(QResizeEvent *e) {
#ifdef VIEW_DEBUG
        egsWarning("In resizeEvent(): size is %d %d old size is: %d %d" " shown: %d\n",width(),height(),e->oldSize().width(), e->oldSize().height(),isShown());
#endif
        resizing = isShown();
        //QDialog::resizeEvent(e);
        QWidget::resizeEvent(e);
    };

    void paintEvent (QPaintEvent *) {
		#ifdef VIEW_DEBUG
        egsWarning("In paintEvent(): size is %d %d resizing is %d\n", width(),height(),resizing);
		#endif
        emit needRepaint(false);
        resizing = false;
    };

    void mouseReleaseEvent (QMouseEvent *event) {
#ifdef VIEW_DEBUG
        egsWarning("In mouseReleaseEvent(): mouse location = (%d, %d)\n", event->x(), event->y());
        egsWarning("  Mouse buttons: %0x\n", event->button());
#endif
        // 500 msec before returning to full resolution (after button released)
        if (navigating) {
            navigationTimer->start(500, TRUE);
            emit regionPicking(xMouse, yMouse);
            navigating=false;
        }
        else if( event->button() == Qt::LeftButton ) {
            egsWarning("release event at %d %d\n",event->x(),event->y());
            emit leftMouseClick(event->x(),event->y());
        }
    }

    //virtual void mousePressEvent ( QMouseEvent * e )
    //virtual void mouseReleaseEvent ( QMouseEvent * e )

    void mouseMoveEvent (QMouseEvent *event) {
        int dx = event->x()-xMouse;
        int dy = event->y()-yMouse;
        xMouse = event->x();
        yMouse = event->y();

        // set up navigation
        if (event->state() & (Qt::LeftButton|Qt::MidButton)) {
            if (!navigating) {
                emit startTransformation();
                navigationTimer->stop();
                navigating=true;
            }
        }

        // navigate
        if (event->state() & Qt::LeftButton) {
            // camera roll
            if (event->state() & Qt::ShiftButton) {
                emit cameraRolling(dx);
            }
            // camera translate
            else if (event->state() & Qt::ControlButton) {
                emit cameraTranslating(dx, dy);
            }
            // camera rotate
            else {
                emit cameraRotation(dx, dy);
            }
        }
		else if (event->state() & Qt::MidButton) {
            // camera zoom
            emit cameraZooming(-dy);
        }
        else {
            // picking
            emit regionPicking(xMouse, yMouse);
        }
    };

    void wheelEvent (QWheelEvent *event) {
        #ifdef VIEW_DEBUG
        egsWarning("In wheelEvent(): mouse location = (%d, %d)\n", event->x(), event->y());
        egsWarning("  Buttons: %0x\n", event->state());
        #endif
        emit startTransformation();
        emit cameraZooming(event->delta()/20);
        // 500 msec before returning to full resolution (after wheel events)
        navigationTimer->start(500, TRUE);
        emit regionPicking(xMouse, yMouse);
    };

    void keyPressEvent (QKeyEvent *event) {
        #ifdef VIEW_DEBUG
        egsWarning("In keyPressEvent()\n");
        #endif
        if (event->key() == Qt::Key_Home) {
            if (event->state() & Qt::AltButton) {
                emit cameraHomeDefining();
            }
            else {
                emit (cameraHoming());
            }
        }
        else if (event->key() == Qt::Key_X) emit putCameraOnAxis('x');
        else if (event->key() == Qt::Key_Y) emit putCameraOnAxis('y');
        else if (event->key() == Qt::Key_Z) emit putCameraOnAxis('z');
        else if (event->key() == Qt::Key_D) emit renderAndDebug();
        else (event->ignore());
    };


signals:

    void changedSize(int w, int h);
    void needRepaint(bool);
	void cameraRotation(int dx, int dy);
	void cameraZooming(int dy);
    void cameraRolling(int dx);
    void cameraTranslating(int dx, int dy);
    void cameraHoming();
    void cameraHomeDefining();
	void startTransformation();
	void endTransformation();
    void putCameraOnAxis(char axis);
    void regionPicking(int x, int y);
    void leftMouseClick(int x, int y);
    void renderAndDebug();


private:

    bool    resizing;
    QTimer  *navigationTimer;
    bool    navigating;

};

#endif
