#include <dialogtest.h>

#include <QCheckBox>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QDialog>

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kglobal.h>


extern "C" KDE_EXPORT int kdemain(int argc, char *argv[])
{
    KAboutData aboutData( "dialogtest", 0, ki18n("dialogtest"),
                          "1.0", ki18n("bla"), KAboutData::License_GPL,
                                             ki18n("(c) 2000 by Christian Esken"));

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::parsedArgs();
    App *app = new App();
    int ret = app->exec();

    delete app;
    return ret;
}

App::App() :KApplication() {
    DialogTest *dialog = new DialogTest();
    dialog->show();
}

DialogTest::DialogTest() : QDialog(  0)
{
    /*
   setCaption("Configure"  );
   setButtons( Ok|Cancel );
   setDefaultButton( Ok );
*/
    /*
   QWidget * frame = new QWidget( this );
   setMainWidget( frame );
*/   
   QWidget * frame = this;
   
   QVBoxLayout* layout = new QVBoxLayout(frame );
   QLabel* qlb = new QLabel( "Configuration of the channels.", frame );
   layout->addWidget(qlb);
   
   QScrollArea* scrollArea = new QScrollArea(frame);
   scrollArea->setWidgetResizable(true); // avoid unnecesary scrollbars
   layout->addWidget(scrollArea);
   
   QWidget* vboxForScrollView = new QWidget();
   QGridLayout* grid = new QGridLayout(vboxForScrollView);
   grid->setHorizontalSpacing(0);

   for (int i=0; i<10; ++i ) {
       QCheckBox* cb = new QCheckBox( "abcdefg abcdefg abcdefg", vboxForScrollView );
            grid->addWidget(cb,i,0);
            cb = new QCheckBox( "", vboxForScrollView );
            grid->addWidget(cb,i,1);
            cb = new QCheckBox( "", vboxForScrollView );
            grid->addWidget(cb,i,2);
    }
    
    scrollArea->setWidget(vboxForScrollView);
}

#include "dialogtest.moc"

