#include <QAction>
#include <QApplication>
#include <QBitmap>
#include <QLabel>
#include <QIcon>
#include <QStylePainter>
#include <QStyleOption>

#include "control/controlobject.h"
#include "widget/wcoverart.h"
#include "widget/wskincolor.h"
#include "library/coverartcache.h"
#include "library/coverartutils.h"
#include "library/dlgcoverartfullsize.h"
#include "util/dnd.h"
#include "util/math.h"

WCoverArt::WCoverArt(QWidget* parent,
                     UserSettingsPointer pConfig,
                     const QString& group)
        : QWidget(parent),
          WBaseWidget(this),
          m_group(group),
          m_pConfig(pConfig),
          m_bEnable(true),
          m_pMenu(new WCoverArtMenu(this)),
          m_pDlgFullSize(new DlgCoverArtFullSize()) {
    // Accept drops if we have a group to load tracks into.
    setAcceptDrops(!m_group.isEmpty());

    CoverArtCache* pCache = CoverArtCache::instance();
    if (pCache != nullptr) {
        connect(pCache, SIGNAL(coverFound(const QObject*,
                                          const CoverInfo&, QPixmap, bool)),
                this, SLOT(slotCoverFound(const QObject*,
                                          const CoverInfo&, QPixmap, bool)));
    }
    connect(m_pMenu, SIGNAL(coverArtSelected(const CoverArt&)),
            this, SLOT(slotCoverArtSelected(const CoverArt&)));
    connect(m_pMenu, SIGNAL(reloadCoverArt()),
            this, SLOT(slotReloadCoverArt()));
}

WCoverArt::~WCoverArt() {
    delete m_pMenu;
    delete m_pDlgFullSize;
}

void WCoverArt::setup(const QDomNode& node, const SkinContext& context) {
    Q_UNUSED(node);
    setMouseTracking(true);

    // Background color
    QColor bgc(255,255,255);
    QString bgColorStr;
    if (context.hasNodeSelectString(node, "BgColor", &bgColorStr)) {
        bgc.setNamedColor(bgColorStr);
        setAutoFillBackground(true);
    }
    QPalette pal = palette();
    pal.setBrush(backgroundRole(), WSkinColor::getCorrectColor(bgc));

    // Foreground color
    QColor m_fgc(0,0,0);
    QString fgColorStr;
    if (context.hasNodeSelectString(node, "FgColor", &fgColorStr)) {
        m_fgc.setNamedColor(fgColorStr);
    }
    bgc = WSkinColor::getCorrectColor(bgc);
    m_fgc = QColor(255 - bgc.red(), 255 - bgc.green(), 255 - bgc.blue());
    pal.setBrush(foregroundRole(), m_fgc);
    setPalette(pal);

    QString defaultCoverStr;
    if (context.hasNodeSelectString(node, "DefaultCover", &defaultCoverStr)) {
        m_defaultCover = QPixmap(defaultCoverStr);
    }

    // If no default cover is specified or we failed to load it, fall back on
    // the resource bundle default cover.
    if (m_defaultCover.isNull()) {
        m_defaultCover = QPixmap(CoverArtUtils::defaultCoverLocation());
    }
    m_defaultCoverScaled = scaledCoverArt(m_defaultCover);
}

void WCoverArt::slotReloadCoverArt() {
    if (m_loadedTrack) {
        CoverArtCache* pCache = CoverArtCache::instance();
        if (pCache) {
            pCache->requestGuessCover(m_loadedTrack);
        }
    }
}

void WCoverArt::slotCoverArtSelected(const CoverArt& art) {
    if (m_loadedTrack) {
        // Will trigger slotTrackCoverArtUpdated().
        m_loadedTrack->setCoverInfo(art.info);
    }
}

void WCoverArt::slotEnable(bool enable) {
    bool wasDisabled = !m_bEnable && enable;
    m_bEnable = enable;

    if (wasDisabled) {
        slotLoadTrack(m_loadedTrack);
    }
    update();
}

void WCoverArt::slotLoadingTrack(TrackPointer pNewTrack, TrackPointer pOldTrack) {
    Q_UNUSED(pNewTrack);
    Q_UNUSED(pOldTrack);
    slotReset();
}

void WCoverArt::slotReset() {
    if (m_loadedTrack) {
        disconnect(m_loadedTrack.data(), SIGNAL(coverArtUpdated()),
                   this, SLOT(slotTrackCoverArtUpdated()));
    }
    m_loadedTrack = TrackPointer();
    m_lastRequestedCover = CoverInfo();
    m_loadedCover = QPixmap();
    m_loadedCoverScaled = QPixmap();
    update();
}

void WCoverArt::slotTrackCoverArtUpdated() {
    CoverArtCache::requestCover(m_loadedTrack.data(), this);
}

void WCoverArt::slotCoverFound(const QObject* pRequestor,
                               const CoverInfo& info, QPixmap pixmap,
                               bool fromCache) {
    Q_UNUSED(info);
    Q_UNUSED(fromCache);
    if (!m_bEnable) {
        return;
    }

    if (pRequestor == this && m_loadedTrack &&
            m_loadedTrack->getCoverInfo().hash == info.hash) {
        qDebug() << "WCoverArt::slotCoverFound" << pRequestor << info
                 << pixmap.size();
        m_loadedCover = pixmap;
        m_loadedCoverScaled = scaledCoverArt(pixmap);
        update();
    }
}

void WCoverArt::slotLoadTrack(TrackPointer pTrack) {
    if (m_loadedTrack) {
        disconnect(m_loadedTrack.data(), SIGNAL(coverArtUpdated()),
                   this, SLOT(slotTrackCoverArtUpdated()));
    }
    m_lastRequestedCover = CoverInfo();
    m_loadedCover = QPixmap();
    m_loadedCoverScaled = QPixmap();
    m_loadedTrack = pTrack;
    if (m_loadedTrack) {
        connect(m_loadedTrack.data(), SIGNAL(coverArtUpdated()),
                this, SLOT(slotTrackCoverArtUpdated()));
    }

    if (!m_bEnable) {
        return;
    }

    slotTrackCoverArtUpdated();
}

QPixmap WCoverArt::scaledCoverArt(const QPixmap& normal) {
    if (normal.isNull()) {
        return QPixmap();
    }
    return normal.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void WCoverArt::paintEvent(QPaintEvent* /*unused*/) {
    QStyleOption option;
    option.initFrom(this);
    QStylePainter painter(this);
    painter.drawPrimitive(QStyle::PE_Widget, option);

    if (!m_bEnable) {
        return;
    }

    QPixmap toDraw = m_loadedCoverScaled;
    if (toDraw.isNull()) {
        toDraw = m_defaultCoverScaled;
    }

    if (!toDraw.isNull()) {
        QSize widgetSize = size();
        QSize pixmapSize = toDraw.size();

        int x = math_max(0, (widgetSize.width() - pixmapSize.width()) / 2);
        int y = math_max(0, (widgetSize.height() - pixmapSize.height()) / 2);
        painter.drawPixmap(x, y, toDraw);
    }
}

void WCoverArt::resizeEvent(QResizeEvent* /*unused*/) {
    m_loadedCoverScaled = scaledCoverArt(m_loadedCover);
    m_defaultCoverScaled = scaledCoverArt(m_defaultCover);
}

void WCoverArt::mousePressEvent(QMouseEvent* event) {
    if (!m_bEnable) {
        return;
    }

    if (event->button() == Qt::RightButton && m_loadedTrack) { // show context-menu
        m_pMenu->setCoverArt(m_loadedTrack->getLocation(), m_lastRequestedCover);
        m_pMenu->popup(event->globalPos());
    } else if (event->button() == Qt::LeftButton) { // init/close fullsize cover
        if (m_pDlgFullSize->isVisible()) {
            m_pDlgFullSize->close();
        } else {
            m_pDlgFullSize->init(m_loadedCover);
        }
    }
}

void WCoverArt::leaveEvent(QEvent* /*unused*/) {
    m_pDlgFullSize->close();
}

void WCoverArt::mouseMoveEvent(QMouseEvent* event) {
    if ((event->buttons() & Qt::LeftButton) && m_loadedTrack) {
        DragAndDropHelper::dragTrack(m_loadedTrack, this, m_group);
    }
}

void WCoverArt::dragEnterEvent(QDragEnterEvent* event) {
    // If group is empty then we are a library cover art widget and we don't
    // accept track drops.
    if (!m_group.isEmpty() &&
            DragAndDropHelper::allowLoadToPlayer(m_group, m_pConfig) &&
            DragAndDropHelper::dragEnterAccept(*event->mimeData(), m_group,
                                               true, false)) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void WCoverArt::dropEvent(QDropEvent *event) {
    // If group is empty then we are a library cover art widget and we don't
    // accept track drops.
    if (!m_group.isEmpty() &&
            DragAndDropHelper::allowLoadToPlayer(m_group, m_pConfig)) {
        QList<QFileInfo> files = DragAndDropHelper::dropEventFiles(
                *event->mimeData(), m_group, true, false);
        if (!files.isEmpty()) {
            event->accept();
            emit(trackDropped(files.at(0).absoluteFilePath(), m_group));
            return;
        }
    }
    event->ignore();
}
