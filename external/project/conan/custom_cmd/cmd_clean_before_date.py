from conan.api.conan_api import ConanAPI
from conan.api.output import ConanOutput, Color
from conan.cli.command import OnceArgument, conan_command
from conans.client.userio import UserInput
from datetime import datetime


recipe_color = Color.BRIGHT_BLUE
removed_color = Color.BRIGHT_YELLOW


@conan_command(group="Custom commands")
def clean_before_date(conan_api: ConanAPI, parser, *args):
    """
    Deletes (from local cache or remotes) all recipe and package revisions before date.
    """
    parser.add_argument('-r', '--remote', action=OnceArgument,
                        help='Will remove from the specified remote')
    parser.add_argument('--force', default=False, action='store_true',
                        help='Remove without requesting a confirmation')
    parser.add_argument('--date', required=True,
                        help='Remove revisions older than this date (format: YYYY-MM-DD)')

    args = parser.parse_args(*args)

    cutoff_date = datetime.strptime(args.date, '%Y-%m-%d')

    def confirmation(message):
        return args.force or ui.request_boolean(message)

    ui = UserInput(non_interactive=False)
    out = ConanOutput()
    remote = conan_api.remotes.get(args.remote) if args.remote else None
    output_remote = remote or "Local cache"

    # Getting all the recipes
    recipes = conan_api.search.recipes("*/*", remote=remote)
    if recipes and not confirmation("Do you want to remove all the recipes revisions and their packages ones, "
                                    "except the latest package revision from the latest recipe one?"):
        return
    for recipe in recipes:
        out.writeln(f"{str(recipe)}", fg=recipe_color)
        all_rrevs = conan_api.list.recipe_revisions(recipe, remote=remote)
        for rrev in all_rrevs:
            rrev_timestamp = datetime.fromtimestamp(rrev.timestamp)
            if rrev_timestamp < cutoff_date:
                conan_api.remove.recipe(rrev, remote=remote)
                out.writeln(f"Removed recipe revision: {rrev.repr_notime()} ({rrev_timestamp.strftime('%Y-%m-%d %H:%M:%S')})"
                            f"and all its package revisions [{output_remote}]", fg=removed_color)
            else:
                packages = conan_api.list.packages_configurations(rrev, remote=remote)
                for package_ref in packages:
                    all_prevs = conan_api.list.package_revisions(package_ref, remote=remote)
                    latest_prev = all_prevs[0] if all_prevs else None
                    for prev in all_prevs:
                        prev_timestamp = datetime.fromtimestamp(prev.timestamp)
                        if prev_timestamp < cutoff_date:
                           conan_api.remove.package(prev, remote=remote)
                           out.writeln(f"Removed package revision: {prev.repr_notime()} ({prev_timestamp.strftime('%Y-%m-%d %H:%M:%S')}) [{output_remote}]", fg=removed_color)
