#ifndef KMixApp_h
#define KMixApp_h

#include <kuniqueapplication.h>

class KMixWindow;

class KMixApp : public KUniqueApplication
{
Q_OBJECT
 public:
    KMixApp();
    ~KMixApp();
    int newInstance ();

    public slots:
    void quitExtended();  // For a hack on visibility()
    static void keepVisibility(bool);

 signals:
    void stopUpdatesOnVisibility();

 private:
    KMixWindow *m_kmix;
    static bool _keepVisibility;
};

#endif
