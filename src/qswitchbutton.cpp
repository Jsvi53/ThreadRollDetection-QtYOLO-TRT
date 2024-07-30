#include "qswitchbutton.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>


SwitchButton::SwitchButton(QWidget *parent)
    : QWidget(parent)
    , m_nHeight(16)
    , m_bChecked(false)
    , m_radius(8.0)
    , m_nMargin(3)
    , m_checkedColor(0, 150, 136)
    , m_thumbColor(Qt::lightGray)
    , m_disabledColor(190, 190, 190)
    , m_backgroundColor(Qt::black)
{
    // mouse slide across, cusor shape change to pointing hand
    setCursor(Qt::PointingHandCursor);

    // connect slot
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    // connect(&m_timer, &QTimer::timeout, this, &SwitchButton::onTimeout);
}

SwitchButton::~SwitchButton() = default;

void SwitchButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);    // unused parameter, for compiler warning

    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    QColor backgroundcolor;
    QColor thumbColor;

    qreal dOpacity;

    if(isEnabled())
    {
        if(m_bChecked)
        {
            backgroundcolor = m_checkedColor;
            thumbColor = m_checkedColor;
            dOpacity = 0.60;
        }else
        {
            backgroundcolor = m_backgroundColor;
            thumbColor = m_thumbColor;
            dOpacity = 0.80;
        }
    }else
    {
        backgroundcolor = m_backgroundColor;
        thumbColor = m_disabledColor;
        dOpacity = 0.260;
    }

    // draw big oval
    painter.setBrush(backgroundcolor);
    painter.setOpacity(dOpacity);
    path.addRoundedRect(QRectF(m_nMargin, m_nMargin, width() - 2 * m_nMargin, height() - 2 * m_nMargin), m_radius, m_radius);
    painter.drawPath(path.simplified());

    // draw small oval
    painter.setBrush(thumbColor);
    painter.setOpacity(1.0);
    painter.drawEllipse(QRectF(m_nX - (m_nHeight / 2), m_nY - (m_nHeight / 2), height(), height()));
}

// mouse press event
void SwitchButton::mousePressEvent(QMouseEvent *event)
{
    if(isEnabled())     //
    {
        if(event->buttons() & Qt::LeftButton)
        {
           event->accept();
        }else
        {
            event->ignore();
        }
    }
}

// mouse release event, changing button state and emitting toggled() signal
void SwitchButton::mouseReleaseEvent(QMouseEvent *event)
{
    if(isEnabled())
    {
        if((event->type() == QMouseEvent::MouseButtonRelease)
            && (event->button() == Qt::LeftButton))
        {
            event->accept();
            m_bChecked = !m_bChecked;
            emit toggled(m_bChecked);
            m_timer.start(10);
        }else
        {
            event->ignore();
        }
    }
}

// change size event
void SwitchButton::resizeEvent(QResizeEvent *event)
{
    m_nX = m_nHeight / 2;
    m_nY = m_nHeight / 2;
    QWidget::resizeEvent(event);
}

// default size
QSize SwitchButton::sizeHint() const
{
    return minimumSizeHint();
}

// smallest size
QSize SwitchButton::minimumSizeHint() const
{
    return QSize(2 * (m_nHeight + m_nMargin), m_nHeight + 2 * m_nMargin);
}

// toggle button state with sliding opt
void SwitchButton::onTimeout()
{
    if (m_bChecked) {
        m_nX += 1;
        if (m_nX >= width() - m_nHeight)
        {
            m_timer.stop();
            m_nX -= 1; // 恢复一下
        }
    } else {
        m_nX -= 1;
        if (m_nX <= m_nHeight / 2)
        {
            m_timer.stop();
            m_nX += 1; // 恢复一下
        }
    }
    update();
}

// return button state: true or false
bool SwitchButton::isToggled() const
{
    return m_bChecked;
}

// set button state
void SwitchButton::setToggled(bool checked)
{
    m_bChecked = checked;
    m_timer.start(10);
}

// set button background color
void SwitchButton::setBackgroundColor(QColor color)
{
    m_backgroundColor = color;
}

// set selected color
void SwitchButton::setCheckedColor(QColor color)
{
    m_checkedColor = color;
}

// set unselected color
void SwitchButton::setDisabledColor(QColor color)
{
    m_disabledColor = color;
}