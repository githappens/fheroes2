/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "campaign_savedata.h"
#include "army.h"
#include "campaign_data.h"
#include "serialize.h"
#include <algorithm>
#include <cassert>

namespace Campaign
{
    CampaignSaveData::CampaignSaveData()
        : _currentScenarioID( 0 )
        , _campaignID( 0 )
        , _daysPassed( 0 )
    {}

    CampaignSaveData & Campaign::CampaignSaveData::Get()
    {
        static CampaignSaveData instance;
        return instance;
    }

    void CampaignSaveData::addCampaignAward( const int awardID )
    {
        _obtainedCampaignAwards.emplace_back( awardID );
    }

    void CampaignSaveData::removeCampaignAward( const int awardID )
    {
        _obtainedCampaignAwards.erase( std::remove( _obtainedCampaignAwards.begin(), _obtainedCampaignAwards.end(), awardID ), _obtainedCampaignAwards.end() );
    }

    void CampaignSaveData::setCurrentScenarioBonus( const ScenarioBonusData & bonus )
    {
        _currentScenarioBonus = bonus;
    }

    void CampaignSaveData::setCurrentScenarioID( const int scenarioID )
    {
        _currentScenarioID = scenarioID;
    }

    void CampaignSaveData::setCampaignID( const int campaignID )
    {
        _campaignID = campaignID;
    }

    void CampaignSaveData::addCurrentMapToFinished()
    {
        const bool isNotDuplicate = std::find( _finishedMaps.begin(), _finishedMaps.end(), _currentScenarioID ) == _finishedMaps.end();
        if ( isNotDuplicate )
            _finishedMaps.emplace_back( _currentScenarioID );
    }

    void CampaignSaveData::addDaysPassed( const uint32_t days )
    {
        _daysPassed += days;
    }

    void CampaignSaveData::reset()
    {
        _finishedMaps.clear();
        _obtainedCampaignAwards.clear();
        _carryOverTroops.clear();
        _currentScenarioID = 0;
        _campaignID = 0;
        _daysPassed = 0;
    }

    void CampaignSaveData::setCarryOverTroops( const Troops & troops )
    {
        _carryOverTroops.clear();

        for ( size_t i = 0; i < troops.Size(); ++i ) {
            _carryOverTroops.emplace_back( *troops.GetTroop( i ) );
        }
    }

    int CampaignSaveData::getLastCompletedScenarioID() const
    {
        assert( !_finishedMaps.empty() );
        return _finishedMaps.back();
    }

    std::vector<Campaign::CampaignAwardData> CampaignSaveData::getObtainedCampaignAwards() const
    {
        std::vector<Campaign::CampaignAwardData> obtainedAwards;

        for ( size_t i = 0; i < _finishedMaps.size(); ++i ) {
            const std::vector<Campaign::CampaignAwardData> awards = Campaign::CampaignAwardData::getCampaignAwardData( _campaignID, _finishedMaps[i] );

            for ( size_t j = 0; j < awards.size(); ++j ) {
                if ( std::find( _obtainedCampaignAwards.begin(), _obtainedCampaignAwards.end(), awards[j]._id ) != _obtainedCampaignAwards.end() )
                    obtainedAwards.emplace_back( awards[j] );
            }
        }

        const std::vector<Campaign::CampaignAwardData> extraAwards = Campaign::CampaignAwardData::getExtraCampaignAwardData( _campaignID );
        for ( const Campaign::CampaignAwardData & award : extraAwards ) {
            if ( std::find( _obtainedCampaignAwards.begin(), _obtainedCampaignAwards.end(), award._id ) != _obtainedCampaignAwards.end() )
                obtainedAwards.emplace_back( award );
        }

        return obtainedAwards;
    }

    StreamBase & operator<<( StreamBase & msg, const CampaignSaveData & data )
    {
        return msg << data._currentScenarioID << data._currentScenarioBonus << data._finishedMaps << data._campaignID << data._daysPassed << data._obtainedCampaignAwards
                   << data._carryOverTroops;
    }

    StreamBase & operator>>( StreamBase & msg, CampaignSaveData & data )
    {
        return msg >> data._currentScenarioID >> data._currentScenarioBonus >> data._finishedMaps >> data._campaignID >> data._daysPassed >> data._obtainedCampaignAwards
               >> data._carryOverTroops;
    }

    ScenarioVictoryCondition getCurrentScenarioVictoryCondition()
    {
        const CampaignSaveData & campaignData = CampaignSaveData::Get();

        const std::vector<ScenarioData> & scenarios = CampaignData::getCampaignData( campaignData.getCampaignID() ).getAllScenarios();
        const int scenarioId = campaignData.getCurrentScenarioID();
        assert( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() );

        if ( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() ) {
            return scenarios[scenarioId].getVictoryCondition();
        }

        return ScenarioVictoryCondition::STANDARD;
    }

    ScenarioLossCondition getCurrentScenarioLossCondition()
    {
        const CampaignSaveData & campaignData = CampaignSaveData::Get();

        const std::vector<ScenarioData> & scenarios = CampaignData::getCampaignData( campaignData.getCampaignID() ).getAllScenarios();
        const int scenarioId = campaignData.getCurrentScenarioID();
        assert( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() );

        if ( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() ) {
            return scenarios[scenarioId].getLossCondition();
        }

        return ScenarioLossCondition::STANDARD;
    }
}
