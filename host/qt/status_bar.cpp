/*
	File: host/qt/status_bar.cpp
	Description: see status_bar.h. Everything is display-only; the editor
	segments re-read the active view (100 ms tick) and the branch segment
	re-reads .git/HEAD (3 s tick) — the same poll pattern as the toast.
*/

#include "status_bar.h"

#include "editor/views/qt/editor_frame.h"
#include "theme.h"
#include "workbench.h"

#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QSvgRenderer>
#include <QTimer>

namespace {

constexpr int kMinBarHeight = 24; // floor; height scales with the font
constexpr int kGlyphGap = 14;	  // left padding + glyph strip before the name

// Extension → display name (keys mirror the tree-sitter grammar table in
// editor/services/highlight/tree_sitter.cpp). Unknown extensions show the
// uppercased extension, VSCode-style.
QString languageDisplayName(const QString &ext)
{
	static const QMap<QString, QString> names = {
		{"c", "C"},
		{"cpp", "C++"},
		{"cc", "C++"},
		{"cxx", "C++"},
		{"h", "C++"},
		{"hpp", "C++"},
		{"mm", "Objective-C++"},
		{"cs", "C#"},
		{"csharp", "C#"},
		{"css", "CSS"},
		{"go", "Go"},
		{"golang", "Go"},
		{"hcl", "HCL"},
		{"tf", "Terraform"},
		{"html", "HTML"},
		{"cshtml", "Razor"},
		{"java", "Java"},
		{"js", "JavaScript"},
		{"jsx", "JavaScript"},
		{"javascript", "JavaScript"},
		{"json", "JSON"},
		{"kt", "Kotlin"},
		{"kts", "Kotlin"},
		{"py", "Python"},
		{"python", "Python"},
		{"rb", "Ruby"},
		{"rs", "Rust"},
		{"rust", "Rust"},
		{"sh", "Shell Script"},
		{"bash", "Shell Script"},
		{"toml", "TOML"},
		{"ts", "TypeScript"},
		{"tsx", "TypeScript React"},
		{"typescript", "TypeScript"},
	};
	const QString key = ext.toLower();
	return names.value(key, ext.isEmpty() ? QString() : ext.toUpper());
}

// Label that always holds the FULL text (so sizeHint claims the real width
// and the layout gives it room when available) but paints an ellipsis when
// it is actually squeezed. minimumSizeHint is zero-width, letting a narrow
// window shrink it instead of widening the window. Eliding at PAINT time
// avoids the resize feedback loop where an elided text shrinks the label,
// which elides further, until "JSON" is stuck as "J..N".
class ElidedLabel : public QLabel
{
  public:
	using QLabel::QLabel;
	QSize minimumSizeHint() const override
	{
		return QSize(0, QLabel::minimumSizeHint().height());
	}

  protected:
	void paintEvent(QPaintEvent *) override
	{
		const QString t = text();
		if (t.isEmpty())
			return;
		QPainter p(this);
		const QRect box = contentsRect();
		p.drawText(box,
				   alignment() | Qt::AlignVCenter,
				   p.fontMetrics().elidedText(t, Qt::ElideMiddle, box.width()));
	}
};

} // namespace

NedStatusBar::NedStatusBar(Workbench *workbench, const Settings &settings, QWidget *parent)
	: QWidget(parent), m_workbench(workbench), m_settings(&settings)
{
	setObjectName(QStringLiteral("NedStatusBar"));

	auto *lay = new QHBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->setSpacing(0);

	// Branch (left). Room for the painted glyph; hidden until a repo is
	// known so the stretch then just pushes the right group to the edge.
	m_branchBox = new QWidget(this);
	m_branchBox->setObjectName(QStringLiteral("NedBranch"));
	auto *branchLay = new QHBoxLayout(m_branchBox);
	branchLay->setSpacing(0);
	m_branch = new ElidedLabel(m_branchBox);
	m_branch->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	branchLay->addWidget(m_branch);
	m_branchBox->hide();
	lay->addWidget(m_branchBox);

	lay->addStretch(1);

	// Right group, VSCode order. ElidedLabels shrink under pressure
	// (narrow windows) instead of widening the window.
	auto segment = [&](const QString &tip) {
		auto *l = new ElidedLabel(this);
		l->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		l->setToolTip(tip);
		lay->addWidget(l);
		return l;
	};
	// Ln and Col are SEPARATE segments with reserved minimum widths: one
	// combined string reflows on every digit change (row 9→10 shoves the
	// column sideways — the flashing). Reserved space = stable layout.
	auto posSegment = [&](const QString &tip, const QString &sample) {
		auto *l = segment(tip);
		// Reserve room for the sample ("Ln 8888") plus the QSS padding.
		l->setMinimumWidth(l->fontMetrics().horizontalAdvance(sample) + 18);
		return l;
	};
	m_ln = posSegment(QObject::tr("Line"), QStringLiteral("Ln 8888"));
	m_col = posSegment(QObject::tr("Column"), QStringLiteral("Col 888"));
	m_indent = segment(QObject::tr("Indentation"));
	m_eol = segment(QObject::tr("Line ending"));
	m_lang = segment(QObject::tr("Language"));
	// Tighter than the others — Ln and Col read as one unit.
	m_ln->setProperty("tight", true);
	m_col->setProperty("tight", true);
	// Font-scaled height + reserved widths (after every segment exists).
	syncFont();

	// Editor segments: fast poll (caret moves have no signal today).
	m_fastTick = new QTimer(this);
	m_fastTick->setInterval(100);
	QObject::connect(m_fastTick, &QTimer::timeout, m_fastTick, [this] { refresh(); });
	m_fastTick->start();
	refresh();

	// Branch: slow poll (HEAD is a tiny file; branch switches are rare).
	m_gitTick = new QTimer(this);
	m_gitTick->setInterval(3000);
	QObject::connect(m_gitTick, &QTimer::timeout, m_gitTick, [this] { refreshBranch(); });
	m_gitTick->start();
}

// ~85% of the app font — status strips are secondary text. Also re-reserves
// the Ln/Col widths for the new metrics. Called at construction and after a
// profile font change (AppHost::applyProfileAppWide).
void NedStatusBar::syncFont()
{
	QFont f = QApplication::font();
	f.setPointSizeF(f.pointSizeF() * 0.85);
	setFont(f);

	// Height FOLLOWS the font (a fixed 26px strip clips once the profile
	// font grows) — text height plus breathing room, over a floor.
	const int h = qMax(kMinBarHeight, fontMetrics().height() + 8);
	if (h != minimumHeight() || h != maximumHeight())
		setFixedHeight(h);

	// Icon strip and the reserved Ln/Col widths re-derive from the new
	// metrics too.
	m_branchBox->layout()->setContentsMargins(kGlyphGap + qRound(h * 0.68) + 3, 0, 0, 0);
	m_ln->setMinimumWidth(
		m_ln->fontMetrics().horizontalAdvance(QStringLiteral("Ln 8888")) + 18);
	m_col->setMinimumWidth(
		m_col->fontMetrics().horizontalAdvance(QStringLiteral("Col 888")) + 18);
}

void NedStatusBar::setWorkspaceRoot(const QString &root)
{
	if (m_root == root)
		return;
	m_root = root;
	refreshBranch();
}

void NedStatusBar::refresh()
{
	EditorFrame *view = m_workbench ? m_workbench->activeView() : nullptr;
	if (!view)
	{
		// No file open: VSCode keeps the strip alive with the defaults
		// (position has nothing to say, but indent/ending/language render).
		m_ln->clear();
		m_col->clear();
		m_indent->setText(QObject::tr("Spaces: 4"));
		m_eol->setText(QStringLiteral("LF"));
		m_lang->setText(QObject::tr("Plain Text"));
		return;
	}

	int row = 0, col = 0;
	view->getCaret(row, col); // 0-based
	m_ln->setText(QObject::tr("Ln %1").arg(row + 1));
	m_col->setText(QObject::tr("Col %2").arg(col + 1));

	// kTabSize expands tabs to 4 monospace cells (editor_frame.h) — a
	// truthful display, not a setting (none exists yet).
	m_indent->setText(QObject::tr("Spaces: 4"));

	m_eol->setText(view->document().lineEnding == "\r\n" ? QStringLiteral("CRLF")
														 : QStringLiteral("LF"));

	QString lang = languageDisplayName(QString::fromStdString(view->languageId()));
	m_lang->setText(lang.isEmpty() ? QObject::tr("Plain Text") : lang);
}

void NedStatusBar::refreshBranch()
{
	m_branchFull = readBranchName();
	m_branchBox->setVisible(!m_branchFull.isEmpty());
	m_branch->setText(m_branchFull);
	update(); // glyph
}

QString NedStatusBar::readBranchName() const
{
	if (m_root.isEmpty())
		return {};
	// Direct repo: .git is a directory, HEAD holds "ref: refs/heads/X".
	const QString refPrefix = QStringLiteral("ref: refs/heads/");
	QFile head(m_root + "/.git/HEAD");
	if (head.open(QIODevice::ReadOnly))
	{
		const QString line = QString::fromUtf8(head.readLine()).trimmed();
		if (line.startsWith(refPrefix))
			return line.mid(refPrefix.size());
		// Detached HEAD: raw sha — show the short form.
		return line.left(7);
	}
	// Worktree/submodule: .git is a FILE with "gitdir: <path>".
	QFile dotGit(m_root + "/.git");
	if (dotGit.open(QIODevice::ReadOnly))
	{
		const QString gitdir = QString::fromUtf8(dotGit.readLine()).trimmed();
		const QString prefix = QStringLiteral("gitdir:");
		if (!gitdir.startsWith(prefix))
			return {};
		QString dir = gitdir.mid(prefix.size()).trimmed();
		QFile wtHead(dir + "/HEAD");
		if (wtHead.open(QIODevice::ReadOnly))
		{
			const QString line = QString::fromUtf8(wtHead.readLine()).trimmed();
			if (line.startsWith(refPrefix))
				return line.mid(refPrefix.size());
			return line.left(7);
		}
	}
	return {};
}

void NedStatusBar::paintEvent(QPaintEvent *event)
{
	QWidget::paintEvent(event);
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	// No separate background — the strip is the window tint (AppHost's
	// paintEvent fill shows through); just a hairline on top.
	p.setPen(QPen(QColor(255, 255, 255, 23), 1.0));
	p.drawLine(QPointF(0, 0.5), QPointF(width(), 0.5));

	if (m_branchFull.isEmpty())
		return;
	// Git-branch icon: the bundled bold SVG rendered DIRECTLY at the exact
	// device pixel size (no QIcon round-trip — its rescales soften the
	// glyph), then tinted with the theme ink (the SVG carries no color).
	// ~0.68 of the bar: a touch above the text height, VSCode proportions.
	static QSvgRenderer renderer(QString::fromStdString(Settings::getAppResourcesPath()) +
								 "/resources/icons/git-branch.svg");
	if (renderer.isValid())
	{
		const qreal dpr = devicePixelRatioF();
		const int logical = qRound(height() * 0.68);
		QImage img(qRound(logical * dpr), qRound(logical * dpr), QImage::Format_ARGB32);
		img.fill(Qt::transparent);
		{
			QPainter ip(&img);
			ip.setRenderHint(QPainter::Antialiasing);
			renderer.render(&ip);
		}
		// Tint: keep the glyph's alpha, replace everything with the ink.
		QPainter tp(&img);
		tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
		tp.fillRect(img.rect(), NedQtTheme::text(*m_settings));
		tp.end();
		img.setDevicePixelRatio(dpr);
		p.drawImage(QPointF(m_branchBox->x() + kGlyphGap, (height() - logical) / 2.0),
					img);
	}
}
