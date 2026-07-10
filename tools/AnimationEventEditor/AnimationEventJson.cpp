#include "AnimationEventJson.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace AnimationEventEditorTool
{
	std::string JsonEscape(std::string_view text)
	{
		std::string result;
		for (const char character : text)
		{
			switch (character)
			{
			case '\\':
				result += "\\\\";
				break;
			case '"':
				result += "\\\"";
				break;
			case '\n':
				result += "\\n";
				break;
			case '\r':
				result += "\\r";
				break;
			case '\t':
				result += "\\t";
				break;
			default:
				result.push_back(character);
				break;
			}
		}
		return result;
	}

	std::string MakeDefaultEventPath(const std::string& fbxPath)
	{
		std::filesystem::path path = std::filesystem::path(fbxPath);
		path.replace_extension(".anim_events.json");
		return path.string();
	}

	namespace
	{
		class AnimationEventJsonParser
		{
		public:
			explicit AnimationEventJsonParser(std::string_view text)
				: m_text(text)
			{
			}

			bool Parse(AnimationEventFileData& output, std::string& error)
			{
				SkipWhitespace();
				if (!Consume('{'))
				{
					return Fail("Expected root object.", error);
				}

				while (true)
				{
					SkipWhitespace();
					if (Consume('}'))
					{
						return true;
					}

					std::string key;
					if (!ParseString(key, error))
					{
						return false;
					}
					if (!Consume(':'))
					{
						return Fail("Expected ':' after object key.", error);
					}

					if (key == "sourceFbx")
					{
						if (!ParseString(output.sourceFbx, error))
						{
							return false;
						}
					}
					else if (key == "events")
					{
						if (!ParseEventsArray(output.events, error))
						{
							return false;
						}
					}
					else if (!SkipValue(error))
					{
						return false;
					}

					SkipWhitespace();
					if (Consume(','))
					{
						continue;
					}
					if (Peek() != '}')
					{
						return Fail("Expected ',' or '}' in root object.", error);
					}
				}
			}

		private:
			void SkipWhitespace()
			{
				while (m_position < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_position])) != 0)
				{
					++m_position;
				}
			}

			char Peek()
			{
				SkipWhitespace();
				return m_position < m_text.size() ? m_text[m_position] : '\0';
			}

			bool Consume(char expected)
			{
				SkipWhitespace();
				if (m_position >= m_text.size() || m_text[m_position] != expected)
				{
					return false;
				}
				++m_position;
				return true;
			}

			bool ParseString(std::string& output, std::string& error)
			{
				SkipWhitespace();
				if (m_position >= m_text.size() || m_text[m_position] != '"')
				{
					return Fail("Expected string.", error);
				}
				++m_position;
				output.clear();

				while (m_position < m_text.size())
				{
					const char character = m_text[m_position++];
					if (character == '"')
					{
						return true;
					}

					if (character != '\\')
					{
						output.push_back(character);
						continue;
					}

					if (m_position >= m_text.size())
					{
						return Fail("Invalid escape sequence.", error);
					}

					const char escaped = m_text[m_position++];
					switch (escaped)
					{
					case '"':
					case '\\':
					case '/':
						output.push_back(escaped);
						break;
					case 'n':
						output.push_back('\n');
						break;
					case 'r':
						output.push_back('\r');
						break;
					case 't':
						output.push_back('\t');
						break;
					default:
						return Fail("Unsupported string escape sequence.", error);
					}
				}

				return Fail("Unterminated string.", error);
			}

			bool ParseNumber(float& output, std::string& error)
			{
				SkipWhitespace();
				const size_t begin = m_position;
				if (m_position < m_text.size() && (m_text[m_position] == '-' || m_text[m_position] == '+'))
				{
					++m_position;
				}
				while (m_position < m_text.size() && std::isdigit(static_cast<unsigned char>(m_text[m_position])) != 0)
				{
					++m_position;
				}
				if (m_position < m_text.size() && m_text[m_position] == '.')
				{
					++m_position;
					while (m_position < m_text.size() && std::isdigit(static_cast<unsigned char>(m_text[m_position])) != 0)
					{
						++m_position;
					}
				}
				if (m_position < m_text.size() && (m_text[m_position] == 'e' || m_text[m_position] == 'E'))
				{
					++m_position;
					if (m_position < m_text.size() && (m_text[m_position] == '-' || m_text[m_position] == '+'))
					{
						++m_position;
					}
					while (m_position < m_text.size() && std::isdigit(static_cast<unsigned char>(m_text[m_position])) != 0)
					{
						++m_position;
					}
				}

				if (begin == m_position)
				{
					return Fail("Expected number.", error);
				}

				try
				{
					output = std::stof(std::string(m_text.substr(begin, m_position - begin)));
				}
				catch (const std::exception&)
				{
					return Fail("Invalid number.", error);
				}
				return true;
			}

			bool ParseEventsArray(std::vector<AnimationEvent>& events, std::string& error)
			{
				if (!Consume('['))
				{
					return Fail("Expected events array.", error);
				}

				events.clear();
				while (true)
				{
					SkipWhitespace();
					if (Consume(']'))
					{
						return true;
					}

					AnimationEvent event{};
					if (!ParseEventObject(event, error))
					{
						return false;
					}
					events.push_back(event);

					SkipWhitespace();
					if (Consume(','))
					{
						continue;
					}
					if (Peek() != ']')
					{
						return Fail("Expected ',' or ']' in events array.", error);
					}
				}
			}

			bool ParseEventObject(AnimationEvent& event, std::string& error)
			{
				if (!Consume('{'))
				{
					return Fail("Expected event object.", error);
				}

				while (true)
				{
					SkipWhitespace();
					if (Consume('}'))
					{
						return true;
					}

					std::string key;
					if (!ParseString(key, error))
					{
						return false;
					}
					if (!Consume(':'))
					{
						return Fail("Expected ':' after event key.", error);
					}

					if (key == "animation")
					{
						std::string value;
						if (!ParseString(value, error))
						{
							return false;
						}
						CopyText(event.animation, value);
					}
					else if (key == "time")
					{
						if (!ParseNumber(event.time, error))
						{
							return false;
						}
						event.time = std::max(0.0f, event.time);
					}
					else if (key == "type")
					{
						std::string value;
						if (!ParseString(value, error))
						{
							return false;
						}
						CopyText(event.type, value);
					}
					else if (key == "name")
					{
						std::string value;
						if (!ParseString(value, error))
						{
							return false;
						}
						CopyText(event.name, value);
					}
					else if (key == "bone")
					{
						std::string value;
						if (!ParseString(value, error))
						{
							return false;
						}
						CopyText(event.bone, value);
					}
					else if (key == "cue")
					{
						std::string value;
						if (!ParseString(value, error))
						{
							return false;
						}
						CopyText(event.cue, value);
					}
					else if (!SkipValue(error))
					{
						return false;
					}

					SkipWhitespace();
					if (Consume(','))
					{
						continue;
					}
					if (Peek() != '}')
					{
						return Fail("Expected ',' or '}' in event object.", error);
					}
				}
			}

			bool SkipValue(std::string& error)
			{
				SkipWhitespace();
				const char valueStart = Peek();
				if (valueStart == '"')
				{
					std::string ignored;
					return ParseString(ignored, error);
				}
				if (valueStart == '{')
				{
					return SkipObject(error);
				}
				if (valueStart == '[')
				{
					return SkipArray(error);
				}
				if (std::isdigit(static_cast<unsigned char>(valueStart)) != 0 || valueStart == '-' || valueStart == '+')
				{
					float ignored{};
					return ParseNumber(ignored, error);
				}
				if (ConsumeLiteral("true") || ConsumeLiteral("false") || ConsumeLiteral("null"))
				{
					return true;
				}

				return Fail("Unsupported JSON value.", error);
			}

			bool SkipObject(std::string& error)
			{
				if (!Consume('{'))
				{
					return false;
				}

				while (true)
				{
					SkipWhitespace();
					if (Consume('}'))
					{
						return true;
					}

					std::string key;
					if (!ParseString(key, error))
					{
						return false;
					}
					if (!Consume(':') || !SkipValue(error))
					{
						return false;
					}
					SkipWhitespace();
					if (Consume(','))
					{
						continue;
					}
					if (Peek() != '}')
					{
						return Fail("Expected ',' or '}' while skipping object.", error);
					}
				}
			}

			bool SkipArray(std::string& error)
			{
				if (!Consume('['))
				{
					return false;
				}

				while (true)
				{
					SkipWhitespace();
					if (Consume(']'))
					{
						return true;
					}
					if (!SkipValue(error))
					{
						return false;
					}
					SkipWhitespace();
					if (Consume(','))
					{
						continue;
					}
					if (Peek() != ']')
					{
						return Fail("Expected ',' or ']' while skipping array.", error);
					}
				}
			}

			bool ConsumeLiteral(std::string_view literal)
			{
				SkipWhitespace();
				if (m_text.substr(m_position, literal.size()) != literal)
				{
					return false;
				}
				m_position += literal.size();
				return true;
			}

			bool Fail(std::string_view message, std::string& error) const
			{
				std::ostringstream stream;
				stream << message << " Offset " << m_position << ".";
				error = stream.str();
				return false;
			}

			std::string_view m_text;
			size_t m_position{};
		};
	}

	std::optional<AnimationEventFileData> LoadAnimationEventFile(const std::string& path, std::string& error)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			error = "Could not open file.";
			return std::nullopt;
		}

		const std::string text{
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>() };
		AnimationEventFileData data;
		AnimationEventJsonParser parser(text);
		if (!parser.Parse(data, error))
		{
			return std::nullopt;
		}
		return data;
	}
}
