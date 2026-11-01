import io
import subprocess

BS = chr(92)

def patch(path, pairs, must=True):
    with io.open(path, encoding='utf-8', newline='') as f:
        s = f.read()
    nl = '\r\n' if '\r\n' in s else '\n'
    ok = True
    for old, new in pairs:
        o = old.replace('\n', nl)
        n = new.replace('\n', nl)
        if o not in s:
            print('MISS in %s: %r' % (path, old[:70]))
            ok = False
            continue
        s = s.replace(o, n, 1)
    with io.open(path, 'w', encoding='utf-8', newline='') as f:
        f.write(s)
    print('patched', path)

# 1. lang key restore (value stays Kilogram-branded)
patch('Telegram/Resources/langs/lang.strings', [
    ('"ayu_UpdateKilogram" = "Update Kilogram";',
     '"ayu_UpdateAyuGram" = "Update Kilogram";'),
    # re-add removed ayu key right before the call-hotkey block
    ('"ayu_CallHotkeySection" = "Calls";',
     '"lng_chat_link_copy" = "Copy";\n"ayu_CallHotkeySection" = "Calls";'),
])

# 2. bottom_info parens
patch('Telegram/SourceFiles/history/view/history_view_bottom_info.cpp', [
    ('''			: edited + ((_data.flags & (Data::Flag::ForwardedDate | Data::Flag::FullDate)))
			? Ui::FormatDateTimeSavedFrom(_data.date)
			: formatMessageTime(_data.date.time()));''',
     '''			: edited + ((_data.flags
				& (Data::Flag::ForwardedDate | Data::Flag::FullDate))
			? Ui::FormatDateTimeSavedFrom(_data.date)
			: formatMessageTime(_data.date.time()));'''),
])

# 3. call_hotkey: calls() returns a reference
patch('Telegram/SourceFiles/ayu/features/call_hotkey/call_hotkey.cpp', [
    ('''	auto &calls = Core::App().calls();
	if (const auto call = calls->currentCall()) {
		call->setMuted(!call->muted());
	} else if (const auto group = calls->currentGroupCall()) {''',
     '''	auto &calls = Core::App().calls();
	if (const auto call = calls.currentCall()) {
		call->setMuted(!call->muted());
	} else if (const auto group = calls.currentGroupCall()) {'''),
])

# 4. context menu icon rename
patch('Telegram/SourceFiles/ayu/ui/context_menu/context_menu.cpp', [
    ('&st::menuIconTTLAny);', '&st::menuIconTTL);'),
])

# 5. donate_info_box: new Earn icon API
patch('Telegram/SourceFiles/ayu/ui/boxes/donate_info_box.cpp', [
    ('''	const auto tonSymbol = emojiHelper.paletteDependent({
		.factory = [=]
		{
			return Ui::Earn::IconCurrencyColored(
				st::boxDividerLabel.style.font,
				st::boxDividerLabel.textFg->c);
		},
		.margin = st::channelEarnCurrencyLearnMargins
	});''',
     '''	const auto tonSymbol = emojiHelper.paletteDependent(
		Ui::Earn::IconCurrencyEmoji({
			.size = st::boxDividerLabel.style.font->height,
			.margin = st::channelEarnCurrencyLearnMargins,
		}));'''),
])

# 6. dialogs_row: upstream activeMatters condition
patch('Telegram/SourceFiles/dialogs/dialogs_row.cpp', [
    ('''		|| activeChanged
		|| _cornerBadgeUserpic->hidden != (hidden ? 1 : 0)''',
     '''		|| (activeMatters && _cornerBadgeUserpic->active != active)
		|| _cornerBadgeUserpic->hidden != (hidden ? 1 : 0)'''),
])

# 7. message_history: new ElementDelegate pure virtual
patch('Telegram/SourceFiles/ayu/ui/message_history/history_inner.h', [
    ('''	HistoryView::GramReadLine *elementGramReadLine() override;''',
     '''	HistoryView::GramReadLine *elementGramReadLine() override;''')
    if False else
    ('''	[[nodiscard]] not_null<Ui::ChatTheme*> theme() const {''',
     '''	HistoryView::GramReadLine *elementGramReadLine() override {
		return nullptr;
	}

	[[nodiscard]] not_null<Ui::ChatTheme*> theme() const {'''),
])

# 8. profile_cover: upstream side (ayu's includes there are dead)
subprocess.run(['git', 'checkout', 'tdesktop/dev', '--',
                'Telegram/SourceFiles/info/profile/info_profile_cover.cpp'], check=True)
print('profile_cover reverted to upstream')

# 9. remove ayu saved_music component from build
subprocess.run(['git', 'rm', '-q', '-f',
                'Telegram/SourceFiles/ayu/ui/components/saved_music.cpp',
                'Telegram/SourceFiles/ayu/ui/components/saved_music.h'], check=True)
print('saved_music component removed')

patch('Telegram/CMakeLists.txt', [
    ('        ayu/ui/components/saved_music.cpp\n', ''),
    ('        ayu/ui/components/saved_music.h\n', ''),
])
print('DONE')
