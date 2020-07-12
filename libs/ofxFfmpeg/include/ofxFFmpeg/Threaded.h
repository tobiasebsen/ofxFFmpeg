#pragma once

namespace ofxFFmpeg {

	template<typename T>
	class Supplier {
	public:
		virtual T * supply() = 0;
	};

	template<typename T>
	class Receiver {
	public:
		virtual void receive(T * t) = 0;
	};

	class Threaded {
	public:

		template<class S, class R>
		void start(Supplier<S> * supplier, Receiver<R> * receiver);
		void stop();
		bool isRunning() const;
		void threadFunc();

	protected:
	};
}