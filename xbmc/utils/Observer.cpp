/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Application.h"
#include "Observer.h"
#include "threads/SingleLock.h"

using namespace std;

Observer::~Observer(void)
{
  StopObserving();
}

void Observer::StopObserving(void)
{
  CSingleLock lock(m_obsCritSection);
  for (unsigned int iObsPtr = 0; iObsPtr < m_observables.size(); iObsPtr++)
    m_observables.at(iObsPtr)->UnregisterObserver(this);
  m_observables.clear();
}

bool Observer::IsObserving(Observable *obs) const
{
  CSingleLock lock(m_obsCritSection);
  return find(m_observables.begin(), m_observables.end(), obs) != m_observables.end();
}

void Observer::RegisterObservable(Observable *obs)
{
  CSingleLock lock(m_obsCritSection);
  if (!IsObserving(obs))
    m_observables.push_back(obs);
}

void Observer::UnregisterObservable(Observable *obs)
{
  CSingleLock lock(m_obsCritSection);
  vector<Observable *>::iterator it = find(m_observables.begin(), m_observables.end(), obs);
  if (it != m_observables.end())
    m_observables.erase(it);
}

Observable::Observable(const CStdString &strObservableName) :
    m_bObservableChanged(false),
    m_strObservableName(strObservableName)
{
}

Observable::~Observable()
{
  StopObserver();
}

Observable &Observable::operator=(const Observable &observable)
{
  CSingleLock lock(m_obsCritSection);

  m_bObservableChanged = observable.m_bObservableChanged;
  m_observers.clear();
  for (unsigned int iObsPtr = 0; iObsPtr < observable.m_observers.size(); iObsPtr++)
    m_observers.push_back(observable.m_observers.at(iObsPtr));

  return *this;
}

void Observable::StopObserver(void)
{
  CSingleLock lock(m_obsCritSection);
  for (unsigned int iObsPtr = 0; iObsPtr < m_observers.size(); iObsPtr++)
    m_observers.at(iObsPtr)->UnregisterObservable(this);
  m_observers.clear();
}

bool Observable::IsObserving(Observer *obs) const
{
  CSingleLock lock(m_obsCritSection);
  return find(m_observers.begin(), m_observers.end(), obs) != m_observers.end();
}

void Observable::RegisterObserver(Observer *obs)
{
  CSingleLock lock(m_obsCritSection);
  if (!IsObserving(obs))
  {
    m_observers.push_back(obs);
    obs->RegisterObservable(this);
  }
}

void Observable::UnregisterObserver(Observer *obs)
{
  CSingleLock lock(m_obsCritSection);
  vector<Observer *>::iterator it = find(m_observers.begin(), m_observers.end(), obs);
  if (it != m_observers.end())
  {
    obs->UnregisterObservable(this);
    m_observers.erase(it);
  }
}

void Observable::NotifyObservers(const CStdString& strMessage /* = "" */)
{
  CSingleLock lock(m_obsCritSection);
  if (m_bObservableChanged && !g_application.m_bStop)
  {
    SendMessage(this, &m_observers, strMessage);
    m_bObservableChanged = false;
  }
}

void Observable::SetChanged(bool SetTo)
{
  CSingleLock lock(m_obsCritSection);
  m_bObservableChanged = SetTo;
}

void Observable::SendMessage(Observable *obs, const vector<Observer *> *observers, const CStdString &strMessage)
{
  for(unsigned int ptr = 0; ptr < observers->size(); ptr++)
  {
    Observer *observer = observers->at(ptr);
    if (observer)
      observer->Notify(obs, strMessage);
  }
}
