#ifndef WTRACKTABLEVIEW_H
#define WTRACKTABLEVIEW_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "configobject.h"
#include "trackinfoobject.h"
#include "library/libraryview.h"
#include "library/trackmodel.h" // Can't forward declare enums
#include "widget/wlibrarytableview.h"
#include "dlgtagfetcher.h"

class ControlObjectThreadMain;
class DlgTrackInfo;
class TrackCollection;

const QString WTRACKTABLEVIEW_VSCROLLBARPOS_KEY = "VScrollBarPos"; /** ConfigValue key for QTable vertical scrollbar position */
const QString LIBRARY_CONFIGVALUE = "[Library]"; /** ConfigValue "value" (wtf) for library stuff */


class WTrackTableView : public WLibraryTableView {
    Q_OBJECT
  public:
    WTrackTableView(QWidget* parent, ConfigObject<ConfigValue>* pConfig,
                    TrackCollection* pTrackCollection, bool sorting = true);
    virtual ~WTrackTableView();
    void contextMenuEvent(QContextMenuEvent * event);
    void onSearchStarting();
    void onSearchCleared();
    void onSearch(const QString& text);
    void onShow();
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void loadSelectedTrack();
    virtual void loadSelectedTrackToGroup(QString group, bool play);

  public slots:
    void loadTrackModel(QAbstractItemModel* model);
    void slotMouseDoubleClicked(const QModelIndex &);
    void slotUnhide();
    void slotPurge();

  private slots:
    void slotRemove();
    void slotHide();
    void slotOpenInFileBrowser();
    void slotShowTrackInfo();
    void slotShowDlgTagFetcher();
    void slotNextTrackInfo();
    void slotNextDlgTagFetcher();
    void slotPrevTrackInfo();
    void slotPrevDlgTagFetcher();
    void slotSendToAutoDJ();
    void slotSendToAutoDJTop();
    void slotReloadTrackMetadata();
    void slotResetPlayed();
    void addSelectionToPlaylist(int iPlaylistId);
    void addSelectionToCrate(int iCrateId);
    void loadSelectionToGroup(QString group, bool play = false);
    void doSortByColumn(int headerSection);
    void slotLockBpm();
    void slotUnlockBpm();
    void slotDoubleBpm();
    void slotHalveBpm();
    void slotTwoThirdsBpm();
    void slotClearBeats();

  private:
    void sendToAutoDJ(bool bTop);
    void showTrackInfo(QModelIndex index);
    void showDlgTagFetcher(QModelIndex index);
    void createActions();
    void dragMoveEvent(QDragMoveEvent * event);
    void dragEnterEvent(QDragEnterEvent * event);
    void dropEvent(QDropEvent * event);
    void lockBpm(bool lock);

    // Mouse move event, implemented to hide the text and show an icon instead
    // when dragging.
    void mouseMoveEvent(QMouseEvent *pEvent);

    // Returns the current TrackModel, or returns NULL if none is set.
    TrackModel* getTrackModel();
    bool modelHasCapabilities(TrackModel::CapabilitiesFlags capability);

    ConfigObject<ConfigValue> * m_pConfig;
    TrackCollection* m_pTrackCollection;

    QSignalMapper m_loadTrackMapper;

    DlgTrackInfo* m_pTrackInfo;
    DlgTagFetcher m_DlgTagFetcher;
    QModelIndex currentTrackInfoIndex;


    ControlObjectThreadMain* m_pNumSamplers;
    ControlObjectThreadMain* m_pNumDecks;
    ControlObjectThreadMain* m_pNumPreviewDecks;

    // Context menu machinery
    QMenu *m_pMenu, *m_pPlaylistMenu, *m_pCrateMenu, *m_pSamplerMenu, *m_pBPMMenu;
    QSignalMapper m_playlistMapper, m_crateMapper, m_deckMapper, m_samplerMapper;

    // Reload Track Metadata Action:
    QAction *m_pReloadMetadataAct;
    QAction *m_pReloadMetadataFromMusicBrainzAct;

    // Load Track to PreviewDeck
    QAction* m_pAddToPreviewDeck;

    // Send to Auto-DJ Action
    QAction *m_pAutoDJAct;
    QAction *m_pAutoDJTopAct;

    // Remove from table
    QAction *m_pRemoveAct;
    QAction *m_pHideAct;
    QAction *m_pUnhideAct;
    QAction *m_pPurgeAct;

    // Reset the played count of selected track or tracks
    QAction* m_pResetPlayedAct;

    // Show track-editor action
    QAction *m_pPropertiesAct;
    QAction *m_pFileBrowserAct;

    // BPM Lock feature
    QAction *m_pBpmLockAction;
    QAction *m_pBpmUnlockAction;

    //BPM changes
    QAction *m_pBpmDoubleAction;
    QAction *m_pBpmHalveAction;
    QAction *m_pBpmTwoThirdsAction;

    // Clear track beats
    QAction* m_pClearBeatsAction;

    bool m_sorting;
};

#endif
