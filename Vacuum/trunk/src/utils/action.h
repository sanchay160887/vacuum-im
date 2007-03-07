#ifndef ACTION_H
#define ACTION_H

#include <QAction>
#include <QHash>
#include <QVariant>
#include "utilsexport.h"

class UTILS_EXPORT Action : 
  public QAction
{
  Q_OBJECT;

public:
  enum DataRoles {
    DR_Parametr1,
    DR_Parametr2,
    DR_Parametr3,
    DR_Parametr4,
    DR_StreamJid,
    DR_UserDefined = 64
  }; 

public:
  Action(int AOrder, QObject *AParent = NULL);
  ~Action();

  int order() const { return FOrder; }
  static int newRole();
  void setData(int ARole, const QVariant &AData);
  QVariant data(int ARole) const;
private:
  static int FNewRole;
  int FOrder;
  QHash<int,QVariant> FData;
};

#endif // ACTION_H
