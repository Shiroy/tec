#pragma once

#include <memory>
#include <set>
#include <iostream>
#include <al.h>
#include <alc.h>

#include "spdlog/spdlog.h"
#include "multiton.hpp"
#include "entity.hpp"
#include "types.hpp"
#include "command-queue.hpp"

namespace tec {
	class VorbisStream;
	typedef Multiton<std::string, std::shared_ptr<VorbisStream>> SoundMap;

	enum AUDIOSOURCE_STATE { PLAYING, PAUSED, STOPPED };

	extern std::map<std::string, std::function<void(std::string)>> file_factories;
	struct AudioSource {
		AudioSource(std::shared_ptr<VorbisStream> stream, bool auto_play) :
			vorbis_stream(stream), source_state(auto_play ? PLAYING : PAUSED), gain(100) { }
		AudioSource() : vorbis_stream(nullptr), source_state(PAUSED), gain(100) { }

		void Out(proto::Component* target) {
			proto::AudioSource* comp = target->mutable_audio_source();
			comp->set_audio_name(this->audio_name);
			comp->set_looping(this->looping);
			comp->set_playing((this->source_state == PLAYING) ? true : false);
			comp->set_volume(this->gain);
		}
		void In(const proto::Component& source) {
			const proto::AudioSource& comp = source.audio_source();
			if (comp.has_audio_name()) {
				this->audio_name = comp.audio_name();
				if (!SoundMap::Has(this->audio_name)) {
					std::string ext = this->audio_name.substr(this->audio_name.find_last_of(".") + 1);
					if (file_factories.find(ext) != file_factories.end()) {
						file_factories[ext](this->audio_name);
					}
				}
				this->vorbis_stream = SoundMap::Get(this->audio_name);
			}
			if (comp.has_looping()) {
				this->looping = comp.looping();
			}
			if (comp.has_playing()) {
				this->source_state = comp.playing() ? PLAYING : PAUSED;
			}
			if (comp.has_volume()) {
				this->gain = comp.volume();
			}
		}

		static ReflectionComponent Reflection(AudioSource* val) {
			ReflectionComponent refcomp;
			Property prop(Property::BOOLEAN);
			(refcomp.properties["Looping"] = prop).Set<bool>(val->looping);
			refcomp.properties["Looping"].update_func = [val] (Property& prop) { val->looping = prop.Get<bool>(); };
			(refcomp.properties["Playing"] = prop).Set<bool>((val->source_state == PLAYING) ? true : false);
			refcomp.properties["Playing"].update_func = [val] (Property& prop) {
				if (prop.Get<bool>()) {
					val->source_state = PLAYING;
				}
				else {
					val->source_state = PAUSED;
				}
			};
			Property dprop(Property::DROPDOWN);
			dropdown_t key_func_mesh = std::make_pair(SoundMap::Keys, val->audio_name);
			(refcomp.properties["Audio Picker"] = dprop).Set(key_func_mesh);
			refcomp.properties["Audio Picker"].update_func = [val] (Property& prop) {
				dropdown_t key_func = prop.Get<dropdown_t>();
				val->audio_name = key_func.second;
				val->vorbis_stream = SoundMap::Get(val->audio_name);
			};
			Property iprop(Property::INTEGER);
			(refcomp.properties["Volume"] = iprop).Set<int>(val->gain);
			refcomp.properties["Volume"].update_func = [val] (Property& prop) { val->gain = prop.Get<int>(); };
			return std::move(refcomp);
		}

		ALuint source;
		ALuint buffer[2];

		bool looping;

		int gain; // Volume

		AUDIOSOURCE_STATE source_state;

		std::string audio_name;

		std::shared_ptr<VorbisStream> vorbis_stream;
	};

	class SoundSystem : public CommandQueue < SoundSystem > {
	public:
		SoundSystem();

		void Update(const double delta);
	private:
		std::shared_ptr<spdlog::logger> _log;
		typedef Multiton<eid, std::shared_ptr<AudioSource>> AudioSourceComponentMap;
		ALCdevice *device;
		ALCcontext *context;

		std::set<std::shared_ptr<AudioSource>> queued_sources;
	};
}
