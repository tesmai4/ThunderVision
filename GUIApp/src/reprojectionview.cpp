#include <QKeyEvent>
#include <QMouseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <iostream>
#include <cmath>
#include <limits>
#include <ud/math/math.hpp>
#include <tdvbasic/exception.hpp>
#include <tdvision/plymeshexporter.hpp>
#include "reprojectionview.hpp"

ReprojectionView::ReprojectionView(QWidget *parent)
    : QGLWidget(parent)
{
    m_reproj = NULL;
    grabKeyboard();
    m_btPressed = false;
    m_empty = true;
    m_zcenter = std::numeric_limits<float>::infinity();
}

void ReprojectionView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    m_camera.lookAt();

    if ( m_reproj != NULL )
    {
        const ud::Aabb &box = m_reproj->box();
        const ud::Vec3f &mi = box.getMin();
        const ud::Vec3f &ma = box.getMax();

#if 0
        glScalef(2.0f/(ma[0] - mi[0]),
                 2.0f/(ma[1] - mi[1]),
                 2.0f/(ma[2] - mi[2]));

        glTranslatef((ma[0] - mi[0])*-0.5f,
                     (ma[1] - mi[1])*-0.5f,
                     (ma[2] - mi[2])*0.5f);
#endif
        float zcenter = (ma[2] + mi[2])*0.5f;
        zcenter = -(zcenter + (ma[2] - zcenter));
        if ( !(m_zcenter < std::numeric_limits<float>::infinity()) )
            m_zcenter = zcenter;
        
        glTranslatef(0.0f, 0.0f, m_zcenter);
        m_reproj->draw();
    }
}

void ReprojectionView::resizeGL(int width, int height)
{
    const float aspect = (width < height)
        ? float(width)/float(height)
        : float(height)/float(width);

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspect, 1.0f, 2000.0f);
    glMatrixMode(GL_MODELVIEW);
}

void ReprojectionView::keyPressEvent(QKeyEvent *event)
{
    if ( m_reproj == NULL || m_empty )
        return ;
    
    const ud::Aabb &box = m_reproj->box();

    const float xmove = (box.getMax()[0] - box.getMin()[0])*0.1f;
    const float ymove = (box.getMax()[1] - box.getMin()[1])*0.1f;
    const float zmove = (box.getMax()[2] - box.getMin()[2])*0.1f;
    
    switch ( event->key() )
    {
    case Qt::Key_W:
        m_camera.move(zmove);
        break;
    case Qt::Key_S:
        m_camera.move(-zmove);
        break;
    case Qt::Key_A:
        m_camera.strife(-xmove);
        break;
    case Qt::Key_D:
        m_camera.strife(xmove);
        break;
    case Qt::Key_R:
        m_camera.move(ymove);
        break;
    case Qt::Key_F:
        m_camera.move(-ymove);
        break;
    default:
        QGLWidget::keyPressEvent(event);
    }
    update();
}

void ReprojectionView::mouseMoveEvent(QMouseEvent *event)
{
    if ( !m_btPressed )
        return ;

    ud::Vec2f pos(event->x(), event->y());

    const float rotRatiox = ud::Math::degToRad(90.0f/float(width()/2));
    const float rotRatioy = ud::Math::degToRad(90.0f/float(height()/2));

    const float xdiff = m_lastMousePos[0] - pos[0];
    const float ydiff = m_lastMousePos[1] - pos[1];

    m_camera.rotateX(ydiff*rotRatiox);
    m_camera.rotateY(xdiff*rotRatioy);

    m_lastMousePos = pos;
    update();
}

void ReprojectionView::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = ud::Vec2f(event->x(), event->y());
    m_btPressed = true;
}

void ReprojectionView::mouseReleaseEvent(QMouseEvent *event)
{
    m_btPressed = false;
}

void ReprojectionView::initializeGL()
{
    glewInit();
    glEnable(GL_DEPTH_TEST);
}

void ReprojectionView::exportMesh()
{
    if ( m_reproj == NULL )
    {
        QMessageBox::warning(this,
                             tr("Export"),
                             tr("No reprojection to export"));
        return ;
    }
    
    QString filename = QFileDialog::getSaveFileName(
        this, tr("Export"), QString(),
        tr("PLY (*.ply)"));

    if ( !filename.isEmpty() )
    {        
        QProgressDialog prgDlg("Exporting", "", 0, 0,
                               NULL, Qt::FramelessWindowHint);
        prgDlg.show();
        try
        {
            tdv::PLYMeshExporter exporter(filename.toStdString());
            
            m_reproj->exportMesh(&exporter);
            prgDlg.close();
        }
        catch (const tdv::Exception &ex)
        {
            prgDlg.close();
            QMessageBox::critical(this,
                                 tr("Can't Export"),
                                  tr(ex.what()));
        }


    }
    
}
