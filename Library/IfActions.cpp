/*
  Copyright (C) 2013 Brian Ferguson

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "Measure.h"
#include "IfActions.h"
#include "Rainmeter.h"
#include "../Common/MathParser.h"
#include "pcre-8.10\config.h"
#include "pcre-8.10\pcre.h"

IfActions::IfActions() :
	m_AboveValue(0.0f),
	m_BelowValue(0.0f),
	m_EqualValue(0),
	m_AboveAction(),
	m_BelowAction(),
	m_EqualAction(),
	m_AboveCommitted(false),
	m_BelowCommitted(false),
	m_EqualCommitted(false),
	m_Conditions(),
	m_ConditionMode(false),
	m_Matches(),
	m_MatchMode(false)
{
}

IfActions::~IfActions()
{
}

void IfActions::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	m_AboveAction = parser.ReadString(section, L"IfAboveAction", L"", false);
	m_AboveValue = parser.ReadFloat(section, L"IfAboveValue", 0.0f);

	m_BelowAction = parser.ReadString(section, L"IfBelowAction", L"", false);
	m_BelowValue = parser.ReadFloat(section, L"IfBelowValue", 0.0f);

	m_EqualAction = parser.ReadString(section, L"IfEqualAction", L"", false);
	m_EqualValue = (int64_t)parser.ReadFloat(section, L"IfEqualValue", 0.0f);
}

void IfActions::ReadConditionOptions(ConfigParser& parser, const WCHAR* section)
{
	// IfCondition options
	m_ConditionMode = parser.ReadBool(section, L"IfConditionMode", false);

	std::wstring condition = parser.ReadString(section, L"IfCondition", L"");
	if (!condition.empty())
	{
		std::wstring tAction = parser.ReadString(section, L"IfTrueAction", L"", false);
		std::wstring fAction = parser.ReadString(section, L"IfFalseAction", L"", false);
		if (!tAction.empty() || !fAction.empty())
		{
			size_t i = 1;
			do
			{
				if (m_Conditions.size() > (i - 1))
				{
					m_Conditions[i - 1].Set(condition, tAction, fAction);
				}
				else
				{
					m_Conditions.emplace_back(condition, tAction, fAction);
				}

				// Check for IfCondition2/IfTrueAction2/IfFalseAction2 ... etc.
				const std::wstring num = std::to_wstring(++i);

				std::wstring key = L"IfCondition" + num;
				condition = parser.ReadString(section, key.c_str(), L"");
				if (condition.empty()) break;

				key = L"IfTrueAction" + num;
				tAction = parser.ReadString(section, key.c_str(), L"", false);
				key = L"IfFalseAction" + num;
				fAction = parser.ReadString(section, key.c_str(), L"", false);
			}
			while (!tAction.empty() || !fAction.empty());
		}
		else
		{
			m_Conditions.clear();
		}
	}
	else
	{
		m_Conditions.clear();
	}

	// IfMatch options
	m_MatchMode = parser.ReadBool(section, L"IfMatchMode", false);

	std::wstring match = parser.ReadString(section, L"IfMatch", L"");
	if (!match.empty())
	{
		std::wstring tAction = parser.ReadString(section, L"IfMatchAction", L"", false);
		std::wstring fAction = parser.ReadString(section, L"IfNotMatchAction", L"", false);
		if (!tAction.empty() || !fAction.empty())
		{
			size_t i = 1;
			do
			{
				if (m_Matches.size() > (i - 1))
				{
					m_Matches[i - 1].Set(match, tAction, fAction);
				}
				else
				{
					m_Matches.emplace_back(match, tAction, fAction);
				}

				// Check for IfCondition2/IfTrueAction2/IfFalseAction2 ... etc.
				const std::wstring num = std::to_wstring(++i);

				std::wstring key = L"IfMatch" + num;
				match = parser.ReadString(section, key.c_str(), L"");
				if (match.empty()) break;

				key = L"IfMatchAction" + num;
				tAction = parser.ReadString(section, key.c_str(), L"", false);
				key = L"IfNotMatchAction" + num;
				fAction = parser.ReadString(section, key.c_str(), L"", false);
			} while (!tAction.empty() || !fAction.empty());
		}
		else
		{
			m_Matches.clear();
		}
	}
	else
	{
		m_Matches.clear();
	}
}

void IfActions::DoIfActions(Measure& measure, double value)
{
	// IfEqual
	if (!m_EqualAction.empty())
	{
		if ((int64_t)value == m_EqualValue)
		{
			if (!m_EqualCommitted)
			{
				m_EqualCommitted = true;		// To avoid infinite loop from !Update
				GetRainmeter().ExecuteCommand(m_EqualAction.c_str(), measure.GetMeterWindow());
			}
		}
		else
		{
			m_EqualCommitted = false;
		}
	}

	// IfAbove
	if (!m_AboveAction.empty())
	{
		if (value > m_AboveValue)
		{
			if (!m_AboveCommitted)
			{
				m_AboveCommitted = true;		// To avoid infinite loop from !Update
				GetRainmeter().ExecuteCommand(m_AboveAction.c_str(), measure.GetMeterWindow());
			}
		}
		else
		{
			m_AboveCommitted = false;
		}
	}

	// IfBelow
	if (!m_BelowAction.empty())
	{
		if (value < m_BelowValue)
		{
			if (!m_BelowCommitted)
			{
				m_BelowCommitted = true;		// To avoid infinite loop from !Update
				GetRainmeter().ExecuteCommand(m_BelowAction.c_str(), measure.GetMeterWindow());
			}
		}
		else
		{
			m_BelowCommitted = false;
		}
	}

	// IfCondition
	int i = 0;
	for (auto& item : m_Conditions)
	{
		++i;
		if (!item.value.empty() && (!item.tAction.empty() || !item.fAction.empty()))
		{
			double result = 0.0f;
			const WCHAR* errMsg = MathParser::Parse(
				item.value.c_str(), &result, measure.GetCurrentMeasureValue, &measure);
			if (errMsg != nullptr)
			{
				if (!item.parseError)
				{
					if (i == 1)
					{
						LogErrorF(&measure, L"%s: IfCondition=%s", errMsg, item.value.c_str());
					}
					else
					{
						LogErrorF(&measure, L"%s: IfCondition%i=%s", errMsg, i, item.value.c_str());
					}
					item.parseError = true;
				}
			}
			else
			{
				item.parseError = false;

				if (result == 1.0f)			// "True"
				{
					item.fCommitted = false;

					if (m_ConditionMode || !item.tCommitted)
					{
						item.tCommitted = true;
						GetRainmeter().ExecuteCommand(item.tAction.c_str(), measure.GetMeterWindow());
					}
				}
				else if (result == 0.0f)	// "False"
				{
					item.tCommitted = false;

					if (m_ConditionMode || !item.fCommitted)
					{
						item.fCommitted = true;
						GetRainmeter().ExecuteCommand(item.fAction.c_str(), measure.GetMeterWindow());
					}
				}
			}
		}
	}
	
	// IfMatch
	i = 0;
	for (auto& item : m_Matches)
	{
		++i;
		if (!item.value.empty() && (!item.tAction.empty() || !item.fAction.empty()))
		{
			const char* error;
			int errorOffset;

			pcre* re = pcre_compile(
				StringUtil::NarrowUTF8(item.value).c_str(),
				PCRE_UTF8,
				&error,
				&errorOffset,
				nullptr);

			if (!re)
			{
				if (!item.parseError)
				{
					if (i == 1)
					{
						LogErrorF(&measure, L"Error: \"%S\" in IfMatch=%s", error, item.value.c_str());
					}
					else
					{
						LogErrorF(&measure, L"Error: \"%S\" in IfMatch%i=%s", error, i, item.value.c_str());
					}

					item.parseError = true;
				}
			}
			else
			{
				item.parseError = false;

				std::string utf8str = StringUtil::NarrowUTF8(measure.GetStringValue());
				int ovector[300];

				int rc = pcre_exec(
					re,
					nullptr,
					utf8str.c_str(),
					(int)utf8str.length(),
					0,
					0,
					ovector,
					(int)_countof(ovector));

				if (rc > 0)		// Match
				{
					item.fCommitted = false;

					if (m_MatchMode || !item.tCommitted)
					{
						item.tCommitted = true;
						GetRainmeter().ExecuteCommand(item.tAction.c_str(), measure.GetMeterWindow());
					}
				}
				else			// Not Match
				{
					item.tCommitted = false;

					if (m_MatchMode || !item.fCommitted)
					{
						item.fCommitted = true;
						GetRainmeter().ExecuteCommand(item.fAction.c_str(), measure.GetMeterWindow());
					}
				}
			}

			// Release memory used for the compiled pattern
			pcre_free(re);
		}
	}
}

void IfActions::SetState(double& value)
{
	// Set IfAction committed state to false if condition is not met with value = 0
	if (m_EqualValue != (int64_t)value)
	{
		m_EqualCommitted = false;
	}

	if (m_AboveValue <= value)
	{
		m_AboveCommitted = false;
	}

	if (m_BelowValue >= value)
	{
		m_BelowCommitted = false;
	}

	for (auto& item : m_Conditions)
	{
		item.tCommitted = false;
		item.fCommitted = false;
	}

	for (auto& item : m_Matches)
	{
		item.tCommitted = false;
		item.fCommitted = false;
	}
}
