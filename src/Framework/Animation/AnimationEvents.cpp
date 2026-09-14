#include "Framework/Animation/AnimationEvents.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace AnimationEvents
{
	namespace
	{
		class Parser
		{
		public:
			explicit Parser(std::string_view text) : m_text(text) {}

			FileData Read()
			{
				FileData data;
				bool schemaSeen = false;
				bool eventsSeen = false;
				Object([&](const std::string& key)
				{
					if (key == "schema")
					{
						const std::string schema = String();
						// Retain read compatibility with event files saved before the EduGame3D rename.
						if (schema != Schema && schema != "open-campus-animation-events-v1")
							Fail("Unsupported animation event schema");
						schemaSeen = true;
					}
					else if (key == "sourceFbx") data.sourceFbx = String();
					else if (key == "events")
					{
						eventsSeen = true;
						Array([&]() { data.events.push_back(ReadEvent()); });
					}
					else Skip(0);
				});
				Whitespace();
				if (m_position != m_text.size()) Fail("Trailing data");
				if (!schemaSeen || !eventsSeen) Fail("Required schema or events is missing");
				return data;
			}

		private:
			[[noreturn]] void Fail(const char* message) const
			{
				throw std::runtime_error(std::string(message) + " at offset " + std::to_string(m_position));
			}
			void Whitespace()
			{
				while (m_position < m_text.size() && (m_text[m_position] == ' ' || m_text[m_position] == '\t' ||
					m_text[m_position] == '\r' || m_text[m_position] == '\n')) ++m_position;
			}
			bool Consume(char character)
			{
				Whitespace();
				if (m_position == m_text.size() || m_text[m_position] != character) return false;
				++m_position;
				return true;
			}
			void Expect(char character)
			{
				if (!Consume(character)) Fail("Unexpected JSON token");
			}
			template <typename Reader> void Object(Reader reader)
			{
				Expect('{');
				if (Consume('}')) return;
				std::unordered_set<std::string> keys;
				for (;;)
				{
					const std::string key = String();
					if (!keys.insert(key).second) Fail("Duplicate object key");
					Expect(':');
					reader(key);
					if (Consume('}')) return;
					Expect(',');
				}
			}
			template <typename Reader> void Array(Reader reader)
			{
				Expect('[');
				if (Consume(']')) return;
				for (;;)
				{
					reader();
					if (Consume(']')) return;
					Expect(',');
				}
			}
			unsigned Hex4()
			{
				unsigned value = 0;
				for (int digit = 0; digit < 4; ++digit)
				{
					if (m_position == m_text.size()) Fail("Incomplete Unicode escape");
					const char c = m_text[m_position++];
					value <<= 4;
					if (c >= '0' && c <= '9') value += c - '0';
					else if (c >= 'a' && c <= 'f') value += c - 'a' + 10;
					else if (c >= 'A' && c <= 'F') value += c - 'A' + 10;
					else Fail("Invalid Unicode escape");
				}
				return value;
			}
			static void AppendUtf8(std::string& text, unsigned code)
			{
				if (code <= 0x7f) text.push_back(static_cast<char>(code));
				else
				{
					if (code > 0xffff) text.push_back(static_cast<char>(0xf0 | (code >> 18)));
					if (code > 0x7ff) text.push_back(static_cast<char>((code > 0xffff ? 0x80 : 0xe0) | ((code >> 12) & 0x3f)));
					text.push_back(static_cast<char>((code > 0x7ff ? 0x80 : 0xc0) | ((code >> 6) & 0x3f)));
					text.push_back(static_cast<char>(0x80 | (code & 0x3f)));
				}
			}
			std::string String()
			{
				Expect('"');
				std::string result;
				while (m_position < m_text.size())
				{
					const char c = m_text[m_position++];
					if (c == '"') return result;
					if (static_cast<unsigned char>(c) < 0x20) Fail("Unescaped control character");
					if (c != '\\') { result.push_back(c); continue; }
					if (m_position == m_text.size()) Fail("Incomplete escape");
					switch (const char escaped = m_text[m_position++])
					{
					case '"': case '\\': case '/': result.push_back(escaped); break;
					case 'b': result.push_back('\b'); break;
					case 'f': result.push_back('\f'); break;
					case 'n': result.push_back('\n'); break;
					case 'r': result.push_back('\r'); break;
					case 't': result.push_back('\t'); break;
					case 'u':
					{
						unsigned code = Hex4();
						if (code >= 0xd800 && code <= 0xdbff)
						{
							if (m_text.substr(m_position, 2) != "\\u") Fail("Missing low surrogate");
							m_position += 2;
							const unsigned low = Hex4();
							if (low < 0xdc00 || low > 0xdfff) Fail("Invalid low surrogate");
							code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
						}
						else if (code >= 0xdc00 && code <= 0xdfff) Fail("Unexpected low surrogate");
						if (code == 0) Fail("Embedded NUL is not supported");
						AppendUtf8(result, code);
						break;
					}
					default: Fail("Invalid string escape");
					}
				}
				Fail("Unterminated string");
			}
			bool Digit() const
			{
				return m_position < m_text.size() && m_text[m_position] >= '0' && m_text[m_position] <= '9';
			}
			void Digits()
			{
				if (!Digit()) Fail("Expected numeric digit");
				do { ++m_position; } while (Digit());
			}
			double Number()
			{
				Whitespace();
				const std::size_t begin = m_position;
				if (m_position < m_text.size() && m_text[m_position] == '-') ++m_position;
				if (m_position < m_text.size() && m_text[m_position] == '0') ++m_position;
				else Digits();
				if (m_position < m_text.size() && m_text[m_position] == '.') { ++m_position; Digits(); }
				if (m_position < m_text.size() && (m_text[m_position] == 'e' || m_text[m_position] == 'E'))
				{
					++m_position;
					if (m_position < m_text.size() && (m_text[m_position] == '+' || m_text[m_position] == '-')) ++m_position;
					Digits();
				}
				double value{};
				const auto result = std::from_chars(m_text.data() + begin, m_text.data() + m_position, value);
				if (result.ec != std::errc{} || !std::isfinite(value)) Fail("Invalid or non-finite number");
				return value;
			}
			Event ReadEvent()
			{
				Event event;
				bool timeSeen = false;
				Object([&](const std::string& key)
				{
					if (key == "time") { event.time = Number(); timeSeen = true; }
					else if (key == "animation") event.animation = String();
					else if (key == "type") event.type = String();
					else if (key == "name") event.name = String();
					else if (key == "bone") event.bone = String();
					else if (key == "cue") event.cue = String();
					else Skip(0);
				});
				if (!timeSeen || event.time < 0.0 || event.animation.empty() || event.type.empty())
					Fail("Event requires nonnegative time, animation and type");
				return event;
			}
			void Skip(unsigned depth)
			{
				if (depth >= 64) Fail("JSON nesting limit exceeded");
				Whitespace();
				if (m_position == m_text.size()) Fail("Missing JSON value");
				const char c = m_text[m_position];
				if (c == '"') { String(); return; }
				if (c == '{') { Object([&](const std::string&) { Skip(depth + 1); }); return; }
				if (c == '[') { Array([&]() { Skip(depth + 1); }); return; }
				for (const std::string_view literal : { "true", "false", "null" })
				{
					if (m_text.substr(m_position, literal.size()) == literal) { m_position += literal.size(); return; }
				}
				Number();
			}
			std::string_view m_text;
			std::size_t m_position{};
		};

		std::string Escape(std::string_view text)
		{
			std::string result = "\"";
			constexpr char hex[] = "0123456789abcdef";
			for (const unsigned char c : text)
			{
				if (c == '"' || c == '\\') { result.push_back('\\'); result.push_back(static_cast<char>(c)); }
				else if (c < 0x20)
				{
					result += "\\u00";
					result.push_back(hex[c >> 4]);
					result.push_back(hex[c & 15]);
				}
				else result.push_back(static_cast<char>(c));
			}
			return result + '"';
		}
	}

	std::optional<FileData> Parse(std::string_view json, std::string& error)
	{
		error.clear();
		try { return Parser(json).Read(); }
		catch (const std::exception& exception) { error = exception.what(); return std::nullopt; }
	}

	std::optional<FileData> Load(const std::filesystem::path& path, std::string& error)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file) { error = "Could not open animation event file"; return std::nullopt; }
		const std::string json{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
		if (file.bad()) { error = "Could not read animation event file"; return std::nullopt; }
		return Parse(json, error);
	}

	std::string Serialize(const FileData& data)
	{
		std::ostringstream output;
		output.imbue(std::locale::classic());
		output << std::setprecision(std::numeric_limits<double>::max_digits10);
		output << "{\n  \"schema\": " << Escape(Schema) << ",\n  \"sourceFbx\": " << Escape(data.sourceFbx) << ",\n  \"events\": [";
		for (std::size_t index = 0; index < data.events.size(); ++index)
		{
			const Event& event = data.events[index];
			output << (index == 0 ? "\n" : ",\n") << "    {\"animation\": " << Escape(event.animation)
				<< ", \"time\": " << event.time << ", \"type\": " << Escape(event.type) << ", \"name\": " << Escape(event.name)
				<< ", \"bone\": " << Escape(event.bone) << ", \"cue\": " << Escape(event.cue) << '}';
		}
		output << "\n  ]\n}\n";
		const std::string json = output.str();
		std::string error;
		if (!Parse(json, error)) throw std::invalid_argument(error);
		return json;
	}

	bool Save(const std::filesystem::path& path, const FileData& data, std::string& error)
	{
		error.clear();
		try
		{
			const std::string json = Serialize(data); // Validate before opening/truncating the target.
			std::ofstream output(path, std::ios::binary);
			output << json;
			output.close();
			if (!output) throw std::runtime_error("Could not write animation event file");
			return true;
		}
		catch (const std::exception& exception) { error = exception.what(); return false; }
	}

	std::vector<Occurrence> Collect(const std::vector<Event>& events, std::string_view animation,
		const AnimationPlaybackInterval& interval, std::size_t maximumOccurrences)
	{
		std::vector<Occurrence> result;
		if (!interval.advanced || interval.durationSeconds <= 0.0) return result;
		for (const Event& event : events)
		{
			if (event.animation != animation || !std::isfinite(event.time) || event.time < 0.0 || event.time > interval.durationSeconds) continue;
			std::uint64_t count = interval.completedLoops;
			if (event.time <= interval.toSeconds && event.time > interval.fromSeconds) ++count;
			else if (event.time > interval.toSeconds && event.time <= interval.fromSeconds && count != 0) --count;
			if (count > maximumOccurrences - result.size()) throw std::length_error("Animation event occurrence budget exceeded");
			const double firstOffset = event.time > interval.fromSeconds ? event.time - interval.fromSeconds
				: interval.durationSeconds - interval.fromSeconds + event.time;
			for (std::uint64_t repetition = 0; repetition < count; ++repetition)
				result.push_back({ event, firstOffset + static_cast<double>(repetition) * interval.durationSeconds });
		}
		std::stable_sort(result.begin(), result.end(), [](const Occurrence& left, const Occurrence& right)
		{
			if (left.offsetSeconds != right.offsetSeconds) return left.offsetSeconds < right.offsetSeconds;
			return left.event.time > right.event.time; // End-of-cycle before next-cycle zero.
		});
		return result;
	}
}
