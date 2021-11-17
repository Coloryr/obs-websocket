#include "RequestHandler.h"

bool IsMediaTimeValid(obs_source_t *input)
{
	auto mediaState = obs_source_media_get_state(input);
	return mediaState == OBS_MEDIA_STATE_PLAYING || mediaState == OBS_MEDIA_STATE_PAUSED;
}

RequestResult RequestHandler::GetMediaInputStatus(const Request& request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput("inputName", statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	json responseData;
	responseData["mediaState"] = Utils::Obs::StringHelper::GetMediaInputState(input);

	if (IsMediaTimeValid(input)) {
		responseData["mediaDuration"] = obs_source_media_get_duration(input);
		responseData["mediaCursor"] = obs_source_media_get_time(input);
	} else {
		responseData["mediaDuration"] = nullptr;
		responseData["mediaCursor"] = nullptr;
	}

	return RequestResult::Success(responseData);
}

RequestResult RequestHandler::SetMediaInputCursor(const Request& request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput("inputName", statusCode, comment);
	if (!(input && request.ValidateNumber("mediaCursor", statusCode, comment, 0)))
		return RequestResult::Error(statusCode, comment);

	if (!IsMediaTimeValid(input))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The media input must be playing or paused in order to set the cursor position.");

	int64_t mediaCursor = request.RequestData["mediaCursor"];

	// Yes, we're setting the time without checking if it's valid. Can't baby everything.
	obs_source_media_set_time(input, mediaCursor);

	return RequestResult::Success();
}

RequestResult RequestHandler::OffsetMediaInputCursor(const Request& request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput("inputName", statusCode, comment);
	if (!(input && request.ValidateNumber("mediaCursorOffset", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	if (!IsMediaTimeValid(input))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The media input must be playing or paused in order to set the cursor position.");

	int64_t mediaCursorOffset = request.RequestData["mediaCursorOffset"];
	int64_t mediaCursor = obs_source_media_get_time(input) + mediaCursorOffset;

	if (mediaCursor < 0)
		mediaCursor = 0;

	obs_source_media_set_time(input, mediaCursor);

	return RequestResult::Success();
}

RequestResult RequestHandler::TriggerMediaInputAction(const Request& request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput("inputName", statusCode, comment);
	if (!(input && request.ValidateString("mediaAction", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	std::string mediaActionString = request.RequestData["mediaAction"];
	auto mediaAction = Utils::Obs::EnumHelper::GetMediaInputAction(mediaActionString);

	switch (mediaAction) {
		default:
		case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NONE:
			return RequestResult::Error(RequestStatus::InvalidRequestParameter, "You have specified an invalid media input action.");
		case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PLAY:
			// Shoutout to whoever implemented this API call like this
			obs_source_media_play_pause(input, false);
			break;
		case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PAUSE:
			obs_source_media_play_pause(input, true);
			break;
		case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_STOP:
			obs_source_media_stop(input);
			break;
		case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_RESTART:
			// I'm only implementing this because I'm nice. I think its a really dumb action.
			obs_source_media_restart(input);
			break;
		case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NEXT:
			obs_source_media_next(input);
			break;
		case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PREVIOUS:
			obs_source_media_previous(input);
			break;
	}

	return RequestResult::Success();
}
