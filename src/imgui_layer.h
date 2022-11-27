#pragma once

#include "layer.h"


namespace Atlas {

	class ImGuiLayer : public Layer {
	public:

		virtual void on_attach() override;
		virtual void on_detach() override;
		virtual void on_update(Timestep ts) override;
		virtual void on_imgui() override;

		void begin();
		void end();

	};

}
